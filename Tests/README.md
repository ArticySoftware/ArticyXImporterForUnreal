# Tests

Automation tests for the Articy X Importer.

## Running locally

A plugin can't run on its own, so these tests run inside a throwaway host project
(`HostProject/`) that just enables the plugin. The runner script generates that host
project, stages the plugin into it, runs the tests headless, and reports pass/fail.

**Windows (PowerShell):**

```powershell
./run-tests.ps1 -UeRoot "C:\Program Files\Epic Games\UE_5.8"
# or set UE_ROOT once and just run ./run-tests.ps1
```

On UE 5.5+ on a memory-tight machine, the Unreal Build Accelerator can throttle the
build almost to a standstill ("Delaying N processes due to memory pressure"). Add
`-NoUBA` to fall back to the standard parallel executor:

```powershell
./run-tests.ps1 -NoUBA
```

**Linux / Mac:**

```bash
UE_ROOT=/path/to/UnrealEngine ./run-tests.sh
```

The script exits non-zero if any test fails. A full report is written to `Report/`.

## Integration tests

The `AXImporter.Integration.*` tests need a host **game project that has already imported
articy content** (a generated database + global variables). They load the real database
and resolve global-variable tokens. They run against such a project in-place (no staging)
via a separate runner:

```powershell
./run-integration-tests.ps1 -UeRoot "C:\Program Files\Epic Games\UE_5.8" -NoUBA
# auto-detects the .uproject two directories above the plugin; override with -Project
```

On Linux / Mac use the shell equivalent:

```bash
UE_ROOT=/path/to/UnrealEngine NO_UBA=1 ./run-integration-tests.sh
# override auto-detection with PROJECT=/path/to/Game.uproject
```

The unit runner filters on `Articy`, so it never picks these up; this runner filters on
`AXImporter.Integration`. Because they need imported content + a live editor world, they run
locally against such a project rather than in generic CI.

## In the editor

With the host project open, the tests also appear in the Session Frontend
(**Tools > Test Automation**) under the **Articy** group, which is handy for
debugging a single test.

## Adding tests

Unit tests live in `../Source/ArticyTests/Private/`. Editor-internal tests that need
private plugin classes live alongside them in `../Source/ArticyEditor/Private/Tests/`.
Each file uses Unreal's automation spec macros and is wrapped in `#if WITH_AUTOMATION_TESTS`.
Name the category `Articy.<Area>.<Thing>` for unit tests, or `AXImporter.Integration.<Area>`
for tests that require imported content, so the right runner picks it up.
