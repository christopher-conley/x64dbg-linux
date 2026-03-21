# x64dbg tests

This is the convention-based automated test tree.

## Source layout

Each test lives under:

```text
src/tests/<rel>/
  test.txt
  optional check.py
  optional *.cpp / plugin sources
```

## Runtime layout

Build output is placed under:

```text
bin/<arch>/tests/
  <rel>.exe
  <rel>/
    test.txt
    *.dp32 / *.dp64
```

## Running

Example:

```powershell
py src/tests/run.py --arch x64
```

Run a single test:

```powershell
py src/tests/run.py --arch x64 issue3808
```

List discovered tests:

```powershell
py src/tests/run.py --arch x64 --list
```

The runner prints each test result as soon as that test finishes, then prints the summary at the end. On failures it dumps only the filtered `[x64dbg-test]` lines from the test log to the console.

By default temporary artifacts are created under:

```text
%TEMP%/x64dbg-tests-<arch>/<randomid>/
```

This keeps all temporary test runs under one parent directory for easier cleanup.

The runner launches one `headless.exe -testing` process per test with:

- isolated `-userdir`
- explicit `-plugin` preloads from the test runtime directory
- `RedirectLog` to a per-test log file
- `-cf` pointing at the built `test.txt`

The canonical pass/fail signal is the final log line emitted by `testfinalize`:

```text
[x64dbg-test] FINAL status=pass asserts=3
[x64dbg-test] FINAL status=fail asserts=0 reason=no_asserts
```
