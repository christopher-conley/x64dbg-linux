# Screen reader checks (UI Automation)

These scripts use Windows UI Automation (UIA) to validate accessibility behavior
without installing a separate screen reader. This is the most script-friendly
approach for Python on Windows.

## Setup (uv)

```powershell
cd c:\CodeBlocks\x64dbg\src\test\screenreader
uv venv
uv sync
```

## Usage

1. Launch x64dbg and open a view that uses an `AbstractTableView`
   (CPU, Log, Memory, etc.).
2. Optionally reorder or hide columns to exercise the column-order bug.
3. Run:

```powershell
uv run x64dbg-screenreader-checks --table-name "CPU"
```

If you are unsure of the table name, omit `--table-name` and the script will
pick the first UIA table it finds under the main window.
If multiple x64dbg windows exist, use `--list-windows` and pass `--window-index`.

## Dump visible UI table contents

This uses UIA to dump only the currently visible cells (based on screen bounds).
By default it dumps all visible table views under the main window.

```powershell
uv run x64dbg-ui-dump --out visible_tables.txt
```

If you want a specific view, set `--table-name` to a substring of the UIA
Table name shown by Inspect.exe.
List-based views (e.g., registers) are included by default.

### Troubleshooting

- If x64dbg is running as Administrator, run the script from an elevated shell.
- If multiple x64dbg windows exist, use `--window-index` to select the right one.
- Use `--list-controls` to dump UIA control-type counts and confirm what UIA is exposing.
- For large tables, use `--max-rows` to cap the output while debugging.

## What it checks

- Bug #1: child indexing off-by-columns
  - Uses the UIA Grid pattern (if available) to compute expected children
    (`headers + rows * cols`) and compares it to the actual UIA child count.
- Bug #2: hit-testing with reordered/hidden columns
  - Uses UIA hit-testing at the center of each header cell and verifies that
    UIA returns an element that belongs to the table.
  - If the UIA HeaderItems are not exposed, it falls back to the top visible
    DataItem row as a header proxy.

If either check fails, the script exits with a non-zero status.
