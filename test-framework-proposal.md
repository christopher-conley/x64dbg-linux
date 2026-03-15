# Test framework proposal

## Summary

Create a small debugger test framework around:

- `src/tests/...` in the source tree
- `bin/x32/tests/...` and `bin/x64/tests/...` in the runtime tree
- `headless.exe` as the default per-test host
- a small Python orchestrator that discovers tests and launches one process per test
- self-asserting tests through:
  - a script command: `testassert`
  - an internal test-plugin API: `DbgTestAssert(...)`

No manifests. No extra test metadata files. No explicit “pass” command.

The Python runner only orchestrates discovery and process launching.
The actual test is run by x64dbg/headless itself.

---

## Goals

- Headless is the default automated host.
- The same test script should also be runnable in the GUI.
- Tests should be isolated from the user’s normal settings/plugins.
- Tests should be discoverable from folder structure alone.
- Script tests, plugin tests, and mixed tests should all use the same model.

## Non-goals

- No manifest system.
- No build orchestration in the Python runner.
- No “reusable test plugin” framework.
- No flush command.

---

## Source tree

New automated test tree:

- `src/tests`

A directory under `src/tests/...` is treated as an automated test if it contains:

- `test.txt`

Optional files:

- `README.md`
- `check.py`

If `test.txt` is absent, the directory is not auto-discovered.

The old `src/test` tree can remain temporarily for legacy/manual content.

---

## Build integration

Yes: add `src/tests` as a subdirectory in the main build and build test artifacts into a known runtime layout.

### Proposed build-side structure

- add `src/tests/cmake.toml`
- add `src/tests` as a subdir from root `cmake.toml`
- build/copy test artifacts into:
  - `bin/x32/tests/...`
  - `bin/x64/tests/...`

The Python runner still does **not** build anything.
It only assumes the runtime tree already exists.

---

## Source tree -> binary tree mapping

Let `<rel>` be the relative path of a test directory under `src/tests`.

Examples:

- `src/tests/issue3808` -> `<rel> = issue3808`
- `src/tests/breakpoints/random_dll` -> `<rel> = breakpoints/random_dll`

### Runtime mapping

For each test `<rel>`:

- main debuggee convention:
  - `bin/<arch>/tests/<rel>.exe`
- per-test runtime directory:
  - `bin/<arch>/tests/<rel>/`
- copied script file:
  - `bin/<arch>/tests/<rel>/test.txt`
- optional in-tree test plugins:
  - `bin/<arch>/tests/<rel>/*.dp32` or `*.dp64`
- optional extra runtime assets:
  - `bin/<arch>/tests/<rel>/...`

### Example

Source:

```text
src/tests/issue3808/
  test.txt
  README.md
  target.cpp
  flagrepro.cpp
```

Runtime:

```text
bin/x64/tests/
  issue3808.exe
  issue3808/
    test.txt
    FlagRepro3808.dp64
```

This allows the test script to simply say:

```text
init tests/issue3808.exe
```

and load/use test-local files from:

```text
tests/issue3808/...
```

---

## Python orchestrator

Suggested entry point:

- `src/tests/run.py`

Suggested commands:

```powershell
py src/tests/run.py list --arch x64
py src/tests/run.py run issue3808 --arch x64
py src/tests/run.py run issue3808 --arch x64 --host gui
py src/tests/run.py run-all --arch x64
```

### Responsibilities

For each discovered test `<rel>`, the runner should:

1. locate `src/tests/<rel>/test.txt`
2. locate `bin/<arch>/tests/<rel>/test.txt`
3. locate test-local plugins in `bin/<arch>/tests/<rel>/*.dpXX`
4. create an isolated userdir for the run unless one is explicitly supplied
5. choose a log file path for the run
6. launch one debugger process for the test
7. wait for completion / timeout
8. decide pass/fail
9. preserve the userdir/log on failure

### Important non-responsibility

The runner should **not** drive the test interactively over stdin.

The test should be driven by debugger startup arguments, especially:

- `-c`
- `-cf`
- `-plugin`
- `-testing`

So the debugger host itself is the thing running the individual test.

---

## Canonical test launch model

The canonical automated launch should be something like:

```powershell
bin/x64/headless.exe \
  -testing \
  -userdir <temp-userdir> \
  -plugin tests/issue3808/FlagRepro3808.dp64 \
  -c "RedirectLog \"<logfile>\"" \
  -cf tests/issue3808/test.txt
```

The GUI/manual equivalent should be:

```powershell
bin/x64/x64dbg.exe \
  -testing \
  -userdir <temp-userdir> \
  -plugin tests/issue3808/FlagRepro3808.dp64 \
  -c "RedirectLog \"<logfile>\"" \
  -cf tests/issue3808/test.txt
```

### Log redirection contract

The log redirection story must be explicit.

The runner should always provide a log file by passing:

```text
-c "RedirectLog \"<logfile>\""
```

before `-cf`.

That gives us one canonical log source for both:

- headless
- GUI

and it captures:

- `log ...` output from scripts
- plugin log output
- assertion failure lines
- normal debugger diagnostics

The runner can still capture stdout/stderr, but the redirected debugger log should be the primary test artifact.

---

## What `-cf` needs in testing mode

This is the key behavior that needs implementation support.

### Current issue

`-cf` currently ends up enqueueing `scriptexec` through the normal debugger command path.
That is useful because it preserves command ordering, but by itself it is not enough for one-shot testing:

- the host still needs to stay alive while the script runs
- headless still needs a clean automatic shutdown point after the script finishes
- GUI should keep its normal message pump behavior

### Important observation

The command queue ordering is actually useful here.

Once `scriptexec` begins executing, the command loop is blocked until `ScriptExecAwait(...)` completes.
That means any command queued **after** `scriptexec` will naturally run only after the test script has finished (or aborted).

So we do **not** need to make the whole startup path strictly synchronous in a way that fights the GUI/message-pump model.

### Proposed `-testing` semantics

Introduce a startup flag:

- `-testing`

This is the dedicated test-run mode.

In `-testing` mode:

1. default plugin autoload is disabled
2. test assertion/failure state is enabled
3. after processing the normal startup arguments, if `-cf` was supplied, x64dbg automatically enqueues a final test command after `scriptexec`
4. headless uses that final command as the graceful one-shot shutdown point

### Proposed final queued command

Add a single debugger command for test finalization, for example:

- `testfinalize`

The runner does **not** need to add this manually if `-testing` does it automatically after `-cf`.

That command ordering would effectively become:

1. `RedirectLog ...`
2. `scriptexec "tests/.../test.txt"`
3. `testfinalize`

Because `scriptexec` blocks until the script is done, `testfinalize` runs at exactly the right time.

### Why this is a better fit

This preserves the existing queued execution model and fits naturally with the GUI message pump.

---

## Startup flags needed for the framework

### 1. `-testing`

Dedicated test-run mode.

Recommended semantics:

- disables normal plugin autoload
- enables test result tracking/assertion counting
- if `-cf` is supplied, automatically queues a final `testfinalize` command after the script command
- enables headless auto-exit from `testfinalize`
- if no `-userdir` is supplied, may create a temporary one automatically for manual/direct use

### 2. `-userdir <abs-path>`

Explicit isolated userdir.

Purpose:

- test settings/database isolation
- reproducible multi-run scenarios when needed

The Python runner will usually create the temp userdir itself and pass it explicitly.

### 3. `-plugin <path>` (repeatable)

Preload test-local plugins before the script/debuggee starts.

Requirements:

- repeatable
- load order is CLI order
- accepts direct `.dp32/.dp64` paths

Optional convenience:

- if a directory is passed, resolve only the conventional single plugin path:
  - `C:\myplugin` -> `C:\myplugin\myplugin.dp64` / `.dp32`
- **not** “load all plugins in that directory”

---

## Plugin loading by path

`plugload` should also be extended to accept direct plugin file paths.

Suggested behavior:

- existing name-based behavior stays
- if the argument looks like a path or ends with `.dp32/.dp64`, load by exact path

This is useful for both manual debugging and tests.

---

## Assertions

We only need an explicit **failure** mechanism.
Success is the happy path.

### Script command: `testassert`

Syntax:

```text
testassert expression
testassert expression, formatstring
```

Because x64dbg commands are comma-separated, this fits naturally.

Semantics:

- increments the global test assertion count
- if the expression is true: continue
- if the expression is false:
  - emit a standardized failure line to the redirected debugger log
  - mark the test as failed
  - abort the current script gracefully

Suggested log prefix:

```text
[x64dbg-test] ASSERT FAIL ...
```

### Command: `testfinalize`

This is the framework-owned finalization command.

Tests do not need to write it manually when `-testing` auto-appends it after `-cf`.

Responsibilities:

- evaluate the final test state
- fail the run if zero assertions executed
- emit one standardized final summary line to the redirected log
- in headless, trigger graceful host shutdown after logging the summary
- in GUI, do **not** auto-close by default

Suggested final log line:

```text
[x64dbg-test] FINAL status=pass asserts=3
[x64dbg-test] FINAL status=fail asserts=0 reason=no_asserts
[x64dbg-test] FINAL status=fail asserts=2 reason=assert_failed
```

This gives the runner a simple, deterministic thing to parse.

### Internal plugin API: `DbgTestAssert(...)`

Needed for in-tree test plugins.

Suggested shape:

```cpp
bool DbgTestAssert(bool condition, const char* fmt = nullptr, ...);
```

Semantics:

- increments the same global assertion count used by `testassert`
- if `condition` is true:
  - return `true`
- if `condition` is false:
  - emit the same standardized failure line
  - mark the test as failed
  - request graceful test termination
  - return `false`

### Why `DbgTestAssert` is needed

A plugin command or callback should not have to reimplement test failure plumbing itself.
It should be able to do:

```cpp
if(!DbgTestAssert(hitCount == 2, "expected 2 hits, got %d", hitCount))
    return false;
```

---

## Graceful termination on assert failure

This is the tricky part and should be designed explicitly.

### Required shared test state

When `-testing` is active, maintain per-process test state such as:

- assertion count
- failure flag
- optional first failure message / failure count

### On `testassert` failure

The script-side command should:

1. increment assertion count
2. set failure flag
3. log the standardized failure line
4. abort the current script gracefully

Existing `DbgScriptAbort()` / script abort plumbing should be reused for this.
After the script returns, the already-queued `testfinalize` command will run and complete shutdown/final result handling.

### On `DbgTestAssert` failure

The plugin-side API should:

1. increment assertion count
2. set failure flag
3. log the same failure line
4. request script abort if a script is active
5. if needed, request a graceful stop of the debuggee so execution unwinds to a stable state

The important part is that `DbgTestAssert(...)` should not try to tear down the whole host immediately.
It should mark failure and trigger graceful unwinding, then let `testfinalize` perform the final completion logic.

A reasonable implementation path is to reuse existing script abort / stop-debug mechanisms rather than inventing a separate test-kill path.

### Finalization flow

The intended ordered flow in `-testing` mode is:

1. `RedirectLog ...`
2. `scriptexec ".../test.txt"`
3. `testfinalize`

If the script aborts or fails, `testfinalize` should still run because it is a separate queued command.
That is important because it centralizes:

- zero-assert detection
- final pass/fail summary logging
- headless auto-exit

---

## Tests must actually assert something

For safety, a test should fail if **no assertions were executed at all**.

That means:

- if neither `testassert` nor `DbgTestAssert` was ever called during the run,
- the run is considered invalid/failing

Reason:

- a script that only launches a target and exits has not verified anything
- “green by doing nothing” is dangerous

This should be part of the final test result evaluation in `-testing` mode.

---

## Settings manipulation

Keep this minimal.

### Command: `settingset`

Syntax:

```text
settingset section, key, value
settingset section, key
```

Semantics:

- with `value`: set/update the setting
- without `value`: unset/remove the key

Behavior:

- call `BridgeSettingSet(...)`
- call `DbgSettingsUpdated()` after a successful change

Examples:

```text
settingset Events, SystemBreakpoint, 1
settingset Engine, InitializeScript, ""
settingset Gui, SomeTransientKey
```

### No flush command

Drop it entirely.
Not needed.

---

## Headless testing behavior

In `-testing` mode, `headless.exe` should act as a one-shot host.

Recommended behavior:

1. parse testing-related startup flags before full startup
2. initialize with isolated userdir
3. disable default plugin autoload
4. preload explicit `-plugin` entries
5. queue startup commands normally (`-c`, then `-cf`)
6. if `-cf` was supplied, automatically queue `testfinalize` after it
7. let the normal command/script machinery run
8. when `testfinalize` executes, emit the final summary line and exit automatically

### How headless should exit

`testfinalize` should not hard-kill the process.

Preferred behavior:

- `testfinalize` logs the final `[x64dbg-test] FINAL ...` line
- if running under the headless host, it requests graceful application shutdown
- headless then follows its normal clean shutdown path

A reasonable implementation is to make headless handle the existing close-application path in a way that maps to its normal shutdown request.
That keeps finalization host-aware without inventing a second shutdown mechanism just for tests.

### Suggested final outcome rules in headless testing mode

A test passes only if:

- the process did not crash
- no assertion failed
- at least one assertion executed
- the startup test script completed successfully

A test fails if:

- any assertion failed
- zero assertions executed
- script execution failed
- the process timed out or crashed

Optionally, headless testing mode can later expose this via dedicated exit codes.

---

## GUI testing behavior

GUI should support the same startup contract:

- `-testing`
- `-userdir`
- `-plugin`
- `-c "RedirectLog ..."`
- `-cf test.txt`

Initial purpose:

- launch the same prepared test scenario for manual inspection/reproduction

Phase 1 does **not** need unattended GUI auto-exit.

In GUI, `testfinalize` should still emit the final summary line, but it should not close the application by default.
That way:

- the same test script runs
- the same log/result plumbing is active
- the user can inspect the GUI state after the scripted setup finishes

---

## Optional fallback checker

Keep optional fallback Python checking.

Convention:

- `src/tests/<rel>/check.py`

Use only when the final validation is awkward to express with `testassert` / `DbgTestAssert`.

The runner can invoke it with the redirected log path and userdir.

---

## Example: `issue3808`

### Source

```text
src/tests/issue3808/
  test.txt
  README.md
  target.cpp
  flagrepro.cpp
```

### Runtime

```text
bin/x64/tests/
  issue3808.exe
  issue3808/
    test.txt
    FlagRepro3808.dp64
```

### `test.txt`

```text
init tests/issue3808.exe
flagrepro3808
```

### Automated launch

```powershell
headless.exe \
  -testing \
  -userdir <tmp-userdir> \
  -plugin tests/issue3808/FlagRepro3808.dp64 \
  -c "RedirectLog \"<logfile>\"" \
  -cf tests/issue3808/test.txt
```

The plugin performs the actual assertions via `DbgTestAssert(...)`.

---

## Exact implementation responsibilities

### 1. Build/layout

- add `src/tests/cmake.toml`
- add `src/tests` from the root `cmake.toml`
- standardize outputs into:
  - `bin/<arch>/tests/<rel>.exe`
  - `bin/<arch>/tests/<rel>/...`

### 2. Startup argument handling

Likely in/around the existing startup argument parsing path:

- add `-testing`
- add `-userdir`
- add repeatable `-plugin`
- when `-testing` and `-cf` are present:
  - queue `scriptexec ...`
  - then queue `testfinalize`
- preserve normal command ordering for `-c` before `-cf`

### 3. Shared test-state plumbing in the debugger

Add per-process test state, enabled by `-testing`, including at least:

- assertion count
- failure flag
- optional failure reason / first failure message
- optional script completion flag if needed

This state should be used by both:

- `testassert`
- `DbgTestAssert(...)`
- `testfinalize`

### 4. New debugger commands / APIs

Add:

- `testassert`
- `testfinalize`
- `settingset`
- internal `DbgTestAssert(...)`

Behavior split:

- `testassert` / `DbgTestAssert(...)`
  - update state
  - log failures
  - initiate graceful unwinding
- `testfinalize`
  - compute final status
  - fail on zero assertions
  - emit final summary line
  - trigger headless shutdown if needed

### 5. Headless host work

Headless needs explicit support for test finalization, not just stdin EOF.

Recommended responsibilities:

- honor `-testing`
- allow the normal queued `-c`/`-cf` model to run
- when debugger-side finalization requests application close, perform the existing graceful shutdown path
- keep stdout/stderr behavior as diagnostics only; redirected log remains canonical

### 6. Python runner

Keep it intentionally small:

- discover tests from `src/tests/**/test.txt`
- derive runtime paths from the `<rel>` convention
- create temp userdir
- choose redirected log path
- find test-local plugins in `bin/<arch>/tests/<rel>/*.dpXX`
- launch one process per test
- wait/timeout
- parse the final `[x64dbg-test] FINAL ...` line
- treat a missing final line as failure
- preserve artifacts on failure

---

## Recommended rollout

### Phase 1: layout and runner

- add `src/tests`
- add `src/tests/cmake.toml`
- wire it into the main build
- build/copy artifacts into `bin/<arch>/tests/...`
- add `src/tests/run.py`
- discover tests from `src/tests/**/test.txt`

### Phase 2: test startup mode

- add `-testing`
- add `-userdir`
- add repeatable `-plugin`
- ensure the runner can prepend `RedirectLog` through startup `-c`
- make `-testing` auto-queue `testfinalize` after `-cf`

### Phase 3: assertion/finalization plumbing

- add shared test state
- add `testassert`
- add `testfinalize`
- add internal `DbgTestAssert(...)`
- add `settingset`
- fail test runs with zero assertions executed
- emit a final standardized summary line from `testfinalize`
- extend `plugload` to accept direct paths

### Phase 4: headless integration

- make headless honor debugger-side close/finalize requests in `-testing`
- make headless auto-exit cleanly after `testfinalize`

### Phase 4: migrate representative tests

- migrate `issue3808`
- add one script-only regression test
- add one mixed script/plugin test involving callbacks

---

## Final recommendation

Use this minimal framework contract:

- source tree: `src/tests/**/test.txt`
- runtime tree:
  - main debuggee: `bin/<arch>/tests/<rel>.exe`
  - per-test dir: `bin/<arch>/tests/<rel>/`
  - test-local plugins: `bin/<arch>/tests/<rel>/*.dpXX`
- runner: small Python orchestrator only
- default host: `headless.exe`
- startup mode: `-testing`
- isolation: explicit `-userdir` from the runner
- plugin preload: repeatable `-plugin`
- canonical output: redirected debugger log via `RedirectLog`
- assertions:
  - script: `testassert`
  - plugin: `DbgTestAssert(...)`
- finalization:
  - framework-owned `testfinalize`
  - emits `[x64dbg-test] FINAL ...`
- failure if:
  - any assert fails
  - script fails
  - process crashes/times out
  - **zero assertions executed**
- GUI: same startup contract, initially for manual inspection rather than unattended automation

This keeps the framework small, explicit, and close to the existing x64dbg execution model.
