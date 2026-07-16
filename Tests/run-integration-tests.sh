#!/usr/bin/env bash
#
# Runs the ArticyXImporter *integration* tests headlessly (Linux/Mac).
#
# Unlike run-tests.sh (which stages the plugin into a throwaway host project for the
# unit tests), this points at a real game project where the plugin lives in-place and
# the demo has been imported. It builds the project's editor target, runs the
# "AXImporter.Integration" automation tests, then parses the JSON report. The editor's own
# exit code is unreliable for test failures, so the report is the source of truth.
#
# Usage:
#   UE_ROOT=/path/to/UnrealEngine ./run-integration-tests.sh
#   UE_ROOT=... PROJECT=/path/to/Game.uproject ./run-integration-tests.sh
#   UE_ROOT=... NO_UBA=1 ./run-integration-tests.sh    # disable UBA on memory-tight machines
#
set -euo pipefail

UE_ROOT="${UE_ROOT:-}"
if [[ -z "$UE_ROOT" ]]; then
	echo "Set the UE_ROOT environment variable to your Unreal Engine root." >&2
	exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
report_dir="$script_dir/ReportIntegration"

# Locate the host game project: use $PROJECT if set, otherwise the single .uproject
# three directories above Tests/ (plugin -> Plugins -> project).
project="${PROJECT:-}"
if [[ -z "$project" ]]; then
	matches=("$script_dir"/../../../*.uproject)
	if [[ ${#matches[@]} -eq 1 && -f "${matches[0]}" ]]; then
		project="$(cd "$(dirname "${matches[0]}")" && pwd)/$(basename "${matches[0]}")"
	fi
fi
if [[ -z "$project" || ! -f "$project" ]]; then
	echo "Set PROJECT=<path-to-.uproject> for a project that has imported articy content." >&2
	exit 2
fi

project_name="$(basename "$project" .uproject)"
target="${project_name}Editor"

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

# Remove any staged plugin copy left by the unit-test runner. It lives inside the
# plugin, so a real game project would otherwise discover it as a duplicate plugin.
rm -rf "$script_dir/HostProject/Plugins"

# Build the game project's editor target (compiles the plugin in-place, incl. tests).
build_args=("$target" "$platform" Development -Project="$project" -WaitMutex)
if [[ -n "${NO_UBA:-}" ]]; then build_args+=(-NoUBA -NoUBALocal); fi
"$UE_ROOT/Engine/Build/BatchFiles/$platform/Build.sh" "${build_args[@]}"

rm -rf "$report_dir"

"$editor" "$project" \
	-ExecCmds="Automation RunTests AXImporter.Integration" \
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
	echo "$failed of $total integration test(s) failed. See $index for details."
	exit 1
fi

echo ""
echo "All $total integration test(s) passed."
exit 0
