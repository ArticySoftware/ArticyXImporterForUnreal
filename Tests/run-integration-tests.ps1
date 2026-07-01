<#
.SYNOPSIS
	Runs the ArticyXImporter *integration* tests against a host game project that has
	already imported articy content (a generated database + global variables).

.DESCRIPTION
	Unlike run-tests.ps1 (which stages the plugin into a throwaway host project for the
	unit tests), this points at a real game project where the plugin lives in-place and
	the demo has been imported. It builds the project's editor target, runs the
	"AXImporter.Integration" automation tests, and parses the report for pass/fail.

.PARAMETER Project
	Path to the .uproject. Defaults to the single .uproject found two directories above
	the plugin root (e.g. the ManiacManfred project that contains this plugin).

.EXAMPLE
	./run-integration-tests.ps1 -UeRoot "C:\Program Files\Epic Games\UE_5.8" -NoUBA
#>
param(
	[string]$UeRoot = $env:UE_ROOT,
	[string]$Project,
	[switch]$NoUBA
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($UeRoot)) {
	Write-Error "Set -UeRoot or the UE_ROOT environment variable to your Unreal Engine root."
	exit 2
}

# Auto-detect the host game project (three levels up from Tests/: plugin -> Plugins -> project).
if ([string]::IsNullOrWhiteSpace($Project)) {
	$found = @(Get-ChildItem "$PSScriptRoot\..\..\..\*.uproject" -ErrorAction SilentlyContinue)
	if ($found.Count -eq 1) { $Project = $found[0].FullName }
}
if ([string]::IsNullOrWhiteSpace($Project) -or -not (Test-Path $Project)) {
	Write-Error "Provide -Project <path-to-.uproject> for a project that has imported articy content."
	exit 2
}

$projectName = [System.IO.Path]::GetFileNameWithoutExtension($Project)
$target      = "${projectName}Editor"
$reportDir   = "$PSScriptRoot\ReportIntegration"

$binDir = "$UeRoot\Engine\Binaries\Win64"
$editor = @("UnrealEditor-Cmd.exe", "UE4Editor-Cmd.exe") |
	ForEach-Object { Join-Path $binDir $_ } |
	Where-Object { Test-Path $_ } |
	Select-Object -First 1
if (-not $editor) {
	Write-Error "Could not find UnrealEditor-Cmd.exe or UE4Editor-Cmd.exe under $binDir."
	exit 2
}

# Remove any staged plugin copy left by the unit-test runner. It lives inside the
# plugin, so a real game project would otherwise discover it as a duplicate plugin.
$staleStage = "$PSScriptRoot\HostProject\Plugins"
if (Test-Path $staleStage) { Remove-Item $staleStage -Recurse -Force }

# Build the game project's editor target (compiles the plugin in-place, incl. tests).
$build = "$UeRoot\Engine\Build\BatchFiles\Build.bat"
$buildArgs = @($target, "Win64", "Development", "-Project=$Project", "-WaitMutex", "-NoHotReloadFromIDE")
if ($NoUBA) { $buildArgs += "-NoUBA"; $buildArgs += "-NoUBALocal" }
& $build @buildArgs
if ($LASTEXITCODE -ne 0) {
	Write-Error "Build failed (exit $LASTEXITCODE)."
	exit 2
}

if (Test-Path $reportDir) { Remove-Item $reportDir -Recurse -Force }

& $editor $Project `
	-ExecCmds="Automation RunTests AXImporter.Integration" `
	-TestExit="Automation Test Queue Empty" `
	-unattended -nopause -nosplash -nullrhi -log `
	-ReportExportPath="$reportDir"

$index = Join-Path $reportDir "index.json"
if (-not (Test-Path $index)) {
	Write-Error "No test report produced at $index. The editor likely failed to launch or compile."
	exit 2
}

$report = Get-Content $index -Raw | ConvertFrom-Json
$failed = @($report.tests | Where-Object { $_.state -ne "Success" })

foreach ($t in $report.tests) {
	$mark = if ($t.state -eq "Success") { "PASS" } else { "FAIL" }
	Write-Host "[$mark] $($t.fullTestPath)"
}

if ($failed.Count -gt 0) {
	Write-Host "`n$($failed.Count) integration test(s) failed." -ForegroundColor Red
	exit 1
}

Write-Host "`nAll $($report.tests.Count) integration test(s) passed." -ForegroundColor Green
exit 0
