# Issue #3808 reproduction (`Script::Flag::Set*`)

This directory contains a minimal, headless reproduction for:

- `gh issue view 3808`
- https://github.com/x64dbg/x64dbg/issues/3808

## Contents

- `flagrepro.cpp` - minimal plugin command callback that exercises `Script::Flag::SetZF/SetCF`
- `target.cpp` - tiny debuggee used by the reproduction
- `run_repro.py` - minimal headless runner that creates a clean sandbox, loads only the repro plugin, runs the command, and parses the result
- `CMakeLists.txt` - standalone build for the plugin and target

## Why the runner creates a sandbox

`headless.exe` auto-loads every plugin in its local `plugins` directory.
In a normal x64dbg build tree that can pull in unrelated plugins, which makes a
minimal repro noisy or unreliable.

The runner copies a small headless runtime into a temporary directory and places
only `FlagRepro3808.dp32/.dp64` in `plugins/`.

## Build

Example x64 build:

```powershell
cmake -S src/test/issue3808 -B src/test/issue3808/build64 -A x64
cmake --build src/test/issue3808/build64 --config Release
```

Example x86 build:

```powershell
cmake -S src/test/issue3808 -B src/test/issue3808/build32 -A Win32
cmake --build src/test/issue3808/build32 --config Release
```

## Run

Inspect the issue with `gh` if needed:

```powershell
gh issue view 3808
```

Run the reproduction against an existing headless build:

### x64

```powershell
py src/test/issue3808/run_repro.py `
  --headless bin/x64/headless.exe `
  --build-dir src/test/issue3808/build64 `
  --config Release
```

### x86

```powershell
py src/test/issue3808/run_repro.py `
  --headless bin/x32/headless.exe `
  --build-dir src/test/issue3808/build32 `
  --config Release
```

Add `--keep-sandbox` if you want to keep the isolated headless directory and full log,
or `--log-file <path>` if you only want to preserve the captured output.

## Expected output

When the bug is present, the runner prints:

```text
result:   REPRODUCED
```

The parsed result line includes:

- `broken=1` when `Script::Flag::Set*` failed to apply the new CPU flag state
- `exact_issue=1` when the setters also returned success, matching the issue report exactly

`exact_issue` can vary by build; `broken=1` is the core regression the runner checks by default.

If the bug is fixed, run with:

```powershell
py src/test/issue3808/run_repro.py ... --expect fixed
```

That mode expects the final state to become `ZF=0` and `CF=1`.
