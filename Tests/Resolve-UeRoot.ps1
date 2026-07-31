#
# Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
#

<#
.SYNOPSIS
	Resolves an Unreal Engine root path for the test runners.

.DESCRIPTION
	Dot-sourced by run-tests.ps1 and run-integration-tests.ps1 so both accept the same
	ways of naming an engine: an explicit root, a version or build identifier looked up
	in the registry, or a .uproject whose EngineAssociation names one of those.
#>

# Reads the EngineAssociation out of a .uproject. That is either a launcher version
# ("5.8") or the identifier of a registered source build.
function Get-UeEngineAssociation {
	param([Parameter(Mandatory)][string]$Project)

	if (-not (Test-Path $Project)) { return $null }

	try {
		$association = (Get-Content $Project -Raw | ConvertFrom-Json).EngineAssociation
	} catch {
		return $null
	}

	if ([string]::IsNullOrWhiteSpace($association)) { return $null }
	return $association
}

# Turns an engine version or build identifier into an installation path.
function Get-UeRootForVersion {
	param([Parameter(Mandatory)][string]$UeVersion)

	# Launcher installs record their path machine-wide.
	$installed = "HKLM:\SOFTWARE\EpicGames\Unreal Engine\$UeVersion"
	if (Test-Path $installed) {
		$dir = (Get-ItemProperty -Path $installed -Name InstalledDirectory -ErrorAction SilentlyContinue).InstalledDirectory
		if ($dir -and (Test-Path $dir)) { return $dir }
	}

	# Not every launcher install writes that key, so check the launcher's own manifest too.
	$manifest = Join-Path $env:ProgramData "Epic\UnrealEngineLauncher\LauncherInstalled.dat"
	if (Test-Path $manifest) {
		try {
			$entry = (Get-Content $manifest -Raw | ConvertFrom-Json).InstallationList |
				Where-Object { $_.AppName -eq "UE_$UeVersion" } |
				Select-Object -First 1
			if ($entry -and $entry.InstallLocation -and (Test-Path $entry.InstallLocation)) {
				return $entry.InstallLocation
			}
		} catch {
			# an unreadable manifest just means this source has nothing to offer
		}
	}

	# Source builds register per user, keyed by the identifier stored in EngineAssociation.
	$builds = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
	if (Test-Path $builds) {
		$dir = (Get-ItemProperty -Path $builds -Name $UeVersion -ErrorAction SilentlyContinue).$UeVersion
		if ($dir -and (Test-Path $dir)) { return $dir }
	}

	# Fall back to the default launcher install location.
	$default = Join-Path $env:ProgramFiles "Epic Games\UE_$UeVersion"
	if (Test-Path $default) { return $default }

	return $null
}

# Picks an engine root from the first option that resolves: an explicit path, a named
# version, then the version the project is associated with.
function Resolve-UeRoot {
	param(
		[string]$UeRoot,
		[string]$UeVersion,
		[string]$Project
	)

	if (-not [string]::IsNullOrWhiteSpace($UeRoot)) {
		if (-not (Test-Path $UeRoot)) {
			Write-Warning "The engine root '$UeRoot' does not exist."
			return $null
		}
		return (Resolve-Path $UeRoot).Path
	}

	if (-not [string]::IsNullOrWhiteSpace($UeVersion)) {
		$resolved = Get-UeRootForVersion $UeVersion
		if (-not $resolved) {
			Write-Warning "No Unreal Engine installation found for version '$UeVersion'."
			return $null
		}
		Write-Host "Using Unreal Engine $UeVersion at $resolved"
		return $resolved
	}

	if (-not [string]::IsNullOrWhiteSpace($Project)) {
		$association = Get-UeEngineAssociation $Project
		if ($association) {
			$resolved = Get-UeRootForVersion $association
			if ($resolved) {
				Write-Host "Using Unreal Engine $association (from $(Split-Path $Project -Leaf)) at $resolved"
				return $resolved
			}
			Write-Warning "The project is associated with engine '$association', which is not installed."
			return $null
		}
	}

	return $null
}
