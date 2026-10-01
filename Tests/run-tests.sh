#!/usr/bin/env bash
#
# Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
#
# Runs the ArticyXImporter automation tests headlessly (Linux/Mac).
#
# Junction the plugin into the host project, launches the editor commandlet to
# run all "Articy" automation tests, then parses the JSON report. The editor's
# own exit code is unreliable for test failures, so the report is the source of
# truth: a non-zero exit code here means one or more tests failed.
#
# Usage:
#   UE_ROOT=/path/to/UnrealEngine ./run-tests.sh
#   UE_VERSION=5.8 ./run-tests.sh               # look the engine up by version instead (Mac)
#   UE_ROOT=... NO_UBA=1 ./run-tests.sh         # disable UBA on memory-tight machines
#   UE_ROOT=... KEEP_STAGING=1 ./run-tests.sh   # keep the staged copy for incremental runs
#
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$script_dir/.." && pwd)"
project="$script_dir/HostProject/HostProject.uproject"
plugin_dir="$script_dir/HostProject/Plugins/ArticyXImporter"
report_dir="$script_dir/Report"

# Determine the host platform as Unreal names it.
if [[ "$(uname -s)" == "Darwin" ]]; then platform="Mac"; else platform="Linux"; fi

# Turns an engine version into an installation path, the way Resolve-UeRoot.ps1 does it from
# the registry on Windows. Only the macOS launcher records its installs somewhere readable;
# on Linux there is nothing to look up, so UE_ROOT stays mandatory there.
resolve_ue_root_for_version() {
	local version="$1"
	[[ "$platform" == "Mac" ]] || return 0

	local default="/Users/Shared/Epic Games/UE_$version"
	if [[ -d "$default" ]]; then echo "$default"; return 0; fi

	# Not every install lands in the default location, so check the launcher's manifest too.
	# InstallLocation precedes AppName within a record, so remember the last one seen and
	# print it once the matching AppName shows up.
	local manifest="/Users/Shared/Epic Games/UnrealEngineLauncher/LauncherInstalled.dat"
	if [[ -f "$manifest" ]]; then
		local dir
		dir=$(awk -v version="$version" '
			/"InstallLocation"/ { loc = $0 }
			/"AppName"/ && index($0, "\"UE_" version "\"") { print loc; exit }
		' "$manifest" | sed -E 's/.*"InstallLocation"[[:space:]]*:[[:space:]]*"(.*)".*/\1/')
		if [[ -n "$dir" && -d "$dir" ]]; then echo "$dir"; return 0; fi
	fi

	return 0
}

UE_ROOT="${UE_ROOT:-}"
if [[ -z "$UE_ROOT" && -n "${UE_VERSION:-}" ]]; then
	UE_ROOT="$(resolve_ue_root_for_version "$UE_VERSION")"
	if [[ -z "$UE_ROOT" ]]; then
		echo "No Unreal Engine installation found for version '$UE_VERSION'." >&2
		exit 2
	fi
	echo "Using Unreal Engine $UE_VERSION at $UE_ROOT"
fi

if [[ -z "$UE_ROOT" ]]; then
	echo "Set the UE_ROOT or UE_VERSION environment variable to name your Unreal Engine." >&2
	exit 2
fi

# Locate the editor binary
editor=""
for name in UnrealEditor-Cmd UE4Editor-Cmd; do
	candidate="$UE_ROOT/Engine/Binaries/$platform/$name"
	if [[ -x "$candidate" ]]; then editor="$candidate"; break; fi
done

if [[ -z "$editor" ]]; then
	echo "Could not find UnrealEditor-Cmd or UE4Editor-Cmd under $UE_ROOT/Engine/Binaries." >&2
	exit 2
fi

# Generate the throwaway host project
host_dir="$script_dir/HostProject"
mkdir -p "$host_dir/Source/HostProject"

cat > "$host_dir/HostProject.uproject" <<'EOF'
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
EOF

cat > "$host_dir/Source/HostProject.Target.cs" <<'EOF'
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
EOF

cat > "$host_dir/Source/HostProjectEditor.Target.cs" <<'EOF'
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
EOF

cat > "$host_dir/Source/HostProject/HostProject.Build.cs" <<'EOF'
using UnrealBuildTool;

public class HostProject : ModuleRules
{
	public HostProject(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
EOF

cat > "$host_dir/Source/HostProject/HostProject.cpp" <<'EOF'
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, HostProject, "HostProject");
EOF

# From here on the plugin exists twice on disk, so the staged copy is removed on the way
# out - including when a step aborts. Left behind, it puts a second .uplugin (and a second
# copy of every class) on the include path of any project that builds this tree.
cleanup_staging() {
	if [[ -z "${KEEP_STAGING:-}" ]]; then
		rm -rf "$host_dir/Plugins"
	fi
}
trap cleanup_staging EXIT

# Stage the plugin into the host project. We mirror-copy rather than symlink
# because the plugin repo contains this host project; linking the whole repo
# would expose the host's own build/target files twice and break the build. The
# Tests folder is excluded for the same reason (the ArticyTests module lives
# under Source/, so it is still included).
mkdir -p "$plugin_dir"
rsync -a --delete \
	--exclude '/Tests' --exclude '/Binaries' --exclude '/Intermediate' \
	--exclude '/Saved' --exclude '/.git' --exclude '/out' \
	"$repo/" "$plugin_dir/"

# Build the host editor target so the plugin (and its test module) are compiled.
build_args=(HostProjectEditor "$platform" Development -Project="$project" -WaitMutex)
if [[ -n "${NO_UBA:-}" ]]; then build_args+=(-NoUBA -NoUBALocal); fi
"$UE_ROOT/Engine/Build/BatchFiles/$platform/Build.sh" "${build_args[@]}"

rm -rf "$report_dir"

"$editor" "$project" \
	-ExecCmds="Automation RunTests Articy" \
	-TestExit="Automation Test Queue Empty" \
	-unattended -nopause -nosplash -nullrhi -log \
	-ReportExportPath="$report_dir"

index="$report_dir/index.json"
if [[ ! -f "$index" ]]; then
	echo "No test report produced at $index. The editor likely failed to launch or compile." >&2
	exit 2
fi

# Parse the report without assuming jq is present: count tests and failures.
total=$(grep -o '"state"' "$index" | wc -l | tr -d ' ')
failed=$(grep -o '"state": *"[^"]*"' "$index" | grep -cv '"Success"' || true)

if [[ "$failed" -gt 0 ]]; then
	echo ""
	echo "$failed of $total test(s) failed. See $index for details."
	exit 1
fi

echo ""
echo "All $total test(s) passed."
exit 0
