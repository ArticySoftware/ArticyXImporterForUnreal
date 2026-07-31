#
# Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
#

<#
.SYNOPSIS
	Runs the ArticyXImporter automation tests headlessly.

.DESCRIPTION
	Junctions the plugin into the host project, launches the editor commandlet to
	run all "Articy" automation tests, then parses the JSON report. The editor's
	own exit code is unreliable for test failures, so the report is the source of
	truth: a non-zero exit code here means one or more tests failed.

.PARAMETER UeRoot
	Path to the Unreal Engine root (the folder containing the Engine directory).
	Defaults to the UE_ROOT environment variable. Takes precedence over -UeVersion.

.PARAMETER UeVersion
	Engine version ("5.8") or the identifier of a registered source build, looked up in
	the registry. Used when -UeRoot is not given; if neither is, the version is read from
	the EngineAssociation of a game project containing this plugin, when there is one.

.PARAMETER NoUBA
	Disable the Unreal Build Accelerator (UE 5.5+). UBA's local executor throttles
	compilation hard on memory-tight machines ("Delaying N processes due to memory
	pressure"). This falls back to the standard parallel executor. Note: both -NoUBA
	and -NoUBALocal are needed - -NoUBA alone only disables the distributed executor.

.PARAMETER KeepStaging
	Keep the staged plugin copy under Tests/HostProject/Plugins instead of removing it
	on exit. Makes repeated runs incremental, at the cost of leaving a second copy of
	the plugin on the include path of any project that builds this tree.

.EXAMPLE
	./run-tests.ps1 -UeRoot "C:\Program Files\Epic Games\UE_5.4"

.EXAMPLE
	./run-tests.ps1 -UeVersion 5.8 -NoUBA
#>
param(
	[string]$UeRoot = $env:UE_ROOT,
	[string]$UeVersion,
	[switch]$NoUBA,
	[switch]$KeepStaging
)

$ErrorActionPreference = "Stop"

. "$PSScriptRoot\Resolve-UeRoot.ps1"

# The plugin usually sits inside a game project (plugin -> Plugins -> project), whose
# engine association names the version to build against.
$hostGame = @(Get-ChildItem "$PSScriptRoot\..\..\..\*.uproject" -ErrorAction SilentlyContinue)
$hostGameProject = if ($hostGame.Count -eq 1) { $hostGame[0].FullName } else { $null }

$UeRoot = Resolve-UeRoot -UeRoot $UeRoot -UeVersion $UeVersion -Project $hostGameProject
if ([string]::IsNullOrWhiteSpace($UeRoot)) {
	Write-Error "Set -UeRoot, -UeVersion or the UE_ROOT environment variable to your Unreal Engine root."
	exit 2
}

$repo       = (Resolve-Path "$PSScriptRoot\..").Path
$project    = "$PSScriptRoot\HostProject\HostProject.uproject"
$pluginDir = "$PSScriptRoot\HostProject\Plugins\ArticyXImporter"
$reportDir  = "$PSScriptRoot\Report"

# Locate the editor binary (UE5 uses UnrealEditor-Cmd, UE4 uses UE4Editor-Cmd).
$binDir = "$UeRoot\Engine\Binaries\Win64"
$editor = @("UnrealEditor-Cmd.exe", "UE4Editor-Cmd.exe") |
	ForEach-Object { Join-Path $binDir $_ } |
	Where-Object { Test-Path $_ } |
	Select-Object -First 1

if (-not $editor) {
	Write-Error "Could not find UnrealEditor-Cmd.exe or UE4Editor-Cmd.exe under $binDir."
	exit 2
}

# Generate the throwaway host project
$hostDir = "$PSScriptRoot\HostProject"
New-Item -ItemType Directory -Path "$hostDir\Source\HostProject" -Force | Out-Null

[System.IO.File]::WriteAllText("$hostDir\HostProject.uproject", @'
{
	"FileVersion": 3,
	"EngineAssociation": "",
	"Category": "",
	"Description": "Headless host project for running ArticyXImporter automation tests.",
	"Modules": [
		{ "Name": "HostProject", "Type": "Runtime", "LoadingPhase": "Default" }
	],
	"Plugins": [
		{ "Name": "ArticyXImporter", "Enabled": true }
	]
}
'@)

[System.IO.File]::WriteAllText("$hostDir\Source\HostProject.Target.cs", @'
using UnrealBuildTool;

public class HostProjectTarget : TargetRules
{
	public HostProjectTarget(TargetInfo target) : base(target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.Add("HostProject");
	}
}
'@)

[System.IO.File]::WriteAllText("$hostDir\Source\HostProjectEditor.Target.cs", @'
using UnrealBuildTool;

public class HostProjectEditorTarget : TargetRules
{
	public HostProjectEditorTarget(TargetInfo target) : base(target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		ExtraModuleNames.Add("HostProject");
	}
}
'@)

[System.IO.File]::WriteAllText("$hostDir\Source\HostProject\HostProject.Build.cs", @'
using UnrealBuildTool;

public class HostProject : ModuleRules
{
	public HostProject(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
'@)

[System.IO.File]::WriteAllText("$hostDir\Source\HostProject\HostProject.cpp", @'
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, HostProject, "HostProject");
'@)

# From here on the plugin exists twice on disk, so the staged copy is removed on the way
# out - including when a step aborts. Left behind, it puts a second .uplugin (and a second
# copy of every class) on the include path of any project that builds this tree.
try {
	# Stage the plugin into the host project. We mirror-copy rather than junction
	# because the plugin repo contains this host project; junctioning the whole repo
	# would expose the host's own build/target files twice and break the build. The
	# Tests folder is excluded for the same reason (the ArticyTests module lives
	# under Source/, so it is still included).
	New-Item -ItemType Directory -Path $pluginDir -Force | Out-Null
	robocopy $repo $pluginDir /MIR /XD "$repo\Tests" "$repo\Binaries" "$repo\Intermediate" "$repo\Saved" "$repo\.git" "$repo\out" /XF "*.sln" /NFL /NDL /NJH /NJS /NP | Out-Null
	if ($LASTEXITCODE -ge 8) { Write-Error "Plugin staging (robocopy) failed (exit $LASTEXITCODE)."; exit 2 }
	$global:LASTEXITCODE = 0

	# Build the host editor target so the plugin (and its test module) are compiled.
	$build = "$UeRoot\Engine\Build\BatchFiles\Build.bat"
	$buildArgs = @("HostProjectEditor", "Win64", "Development", "-Project=$project", "-WaitMutex", "-NoHotReloadFromIDE")
	if ($NoUBA) { $buildArgs += "-NoUBA"; $buildArgs += "-NoUBALocal" }
	& $build @buildArgs
	if ($LASTEXITCODE -ne 0) {
		Write-Error "Build failed (exit $LASTEXITCODE)."
		exit 2
	}

	if (Test-Path $reportDir) { Remove-Item $reportDir -Recurse -Force }

	& $editor $project `
		-ExecCmds="Automation RunTests Articy" `
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
		Write-Host "`n$($failed.Count) test(s) failed." -ForegroundColor Red
		exit 1
	}

	Write-Host "`nAll $($report.tests.Count) test(s) passed." -ForegroundColor Green
	exit 0
}
finally {
	$stagedPlugins = "$hostDir\Plugins"
	if (-not $KeepStaging -and (Test-Path $stagedPlugins)) {
		Remove-Item $stagedPlugins -Recurse -Force -ErrorAction SilentlyContinue
	}
}
