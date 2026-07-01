#!/usr/bin/env bash
#
# Runs the ArticyXImporter automation tests headlessly (Linux/Mac).
#
# Symlinks the plugin into the host project, launches the editor commandlet to
# run all "Articy" automation tests, then parses the JSON report. The editor's
# own exit code is unreliable for test failures, so the report is the source of
# truth: a non-zero exit code here means one or more tests failed.
#
# Usage: UE_ROOT=/path/to/UnrealEngine ./run-tests.sh
#
set -euo pipefail

UE_ROOT="${UE_ROOT:-}"
if [[ -z "$UE_ROOT" ]]; then
	echo "Set the UE_ROOT environment variable to your Unreal Engine root." >&2
	exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$script_dir/.." && pwd)"
project="$script_dir/HostProject/HostProject.uproject"
plugin_dir="$script_dir/HostProject/Plugins/ArticyXImporter"
report_dir="$script_dir/Report"

# Determine the host platform as Unreal names it.
if [[ "$(uname -s)" == "Darwin" ]]; then platform="Mac"; else platform="Linux"; fi

# Locate the editor binary (UE5 uses UnrealEditor-Cmd, UE4 uses UE4Editor-Cmd).
editor=""
for name in UnrealEditor-Cmd UE4Editor-Cmd; do
	candidate="$UE_ROOT/Engine/Binaries/$platform/$name"
	if [[ -x "$candidate" ]]; then editor="$candidate"; break; fi
done

if [[ -z "$editor" ]]; then
	echo "Could not find UnrealEditor-Cmd or UE4Editor-Cmd under $UE_ROOT/Engine/Binaries." >&2
	exit 2
fi

# Generate the throwaway host project (a build artifact, not committed to the repo).
# It is the smallest project that can compile the plugin from the command line: a
# minimal primary game module plus its Game/Editor target files.
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
"$UE_ROOT/Engine/Build/BatchFiles/$platform/Build.sh" \
	HostProjectEditor "$platform" Development -Project="$project" -WaitMutex

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
