from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass
from typing import Optional

from pywinauto import Desktop
from pywinauto.controls.uiawrapper import UIAWrapper

from ui_common import (
    find_tables,
    iter_descendants,
    list_matching_windows,
    select_window,
)


@dataclass
class CheckResult:
    ok: bool
    details: str


def _find_table(root: UIAWrapper, name_hint: Optional[str]) -> Optional[UIAWrapper]:
    tables = find_tables(root, name_hint)
    if not tables:
        return None
    return tables[0]


def _try_get_grid_dimensions(table: UIAWrapper) -> Optional[tuple[int, int]]:
    try:
        grid = table.iface_grid
        rows = int(grid.CurrentRowCount)
        cols = int(grid.CurrentColumnCount)
        return rows, cols
    except Exception:
        return None


def _header_items(table: UIAWrapper) -> list[UIAWrapper]:
    headers = []
    for element in iter_descendants(table):
        if element.element_info.control_type in {"HeaderItem", "Header"}:
            headers.append(element)
    headers.sort(key=lambda item: item.rectangle().left)
    return headers


def _top_row_items(table: UIAWrapper) -> list[UIAWrapper]:
    items = []
    min_top = None
    for element in iter_descendants(table):
        if element.element_info.control_type != "DataItem":
            continue
        try:
            rect = element.rectangle()
        except Exception:
            continue
        if rect.width() <= 0 or rect.height() <= 0:
            continue
        if min_top is None or rect.top < min_top:
            min_top = rect.top
    if min_top is None:
        return []
    tolerance = 2
    for element in iter_descendants(table):
        if element.element_info.control_type != "DataItem":
            continue
        try:
            rect = element.rectangle()
        except Exception:
            continue
        if rect.width() <= 0 or rect.height() <= 0:
            continue
        if abs(rect.top - min_top) <= tolerance:
            items.append(element)
    items.sort(key=lambda item: item.rectangle().left)
    return items


def _is_descendant_of(child: UIAWrapper, ancestor: UIAWrapper) -> bool:
    current = child
    while True:
        try:
            if current.element_info.runtime_id == ancestor.element_info.runtime_id:
                return True
        except Exception:
            return False
        try:
            parent = current.parent()
        except Exception:
            return False
        if parent is None:
            return False
        current = parent


def _check_child_count(table: UIAWrapper) -> CheckResult:
    dims = _try_get_grid_dimensions(table)
    if not dims:
        return CheckResult(
            ok=False,
            details="Grid pattern not available; cannot validate child count.",
        )
    rows, cols = dims
    expected = rows * cols
    try:
        actual = len(table.children())
    except Exception as exc:
        return CheckResult(ok=False, details=f"Failed to read child count: {exc}")
    if actual != expected:
        return CheckResult(
            ok=False,
            details=f"Child count mismatch: expected {expected} (rows={rows}, cols={cols}), got {actual}.",
        )
    return CheckResult(ok=True, details=f"Child count OK ({actual}).")


def _check_header_hit_test(table: UIAWrapper) -> CheckResult:
    headers = _header_items(table)
    used_fallback = False
    if not headers:
        headers = _top_row_items(table)
        used_fallback = True
    if not headers:
        return CheckResult(
            ok=False, details="No header or top-row items found for hit-testing."
        )
    try:
        table.top_level_parent().set_focus()
    except Exception:
        pass
    try:
        table.set_focus()
    except Exception:
        pass
    time.sleep(0.05)
    desktop = Desktop(backend="uia")
    failures = []
    for index, header in enumerate(headers):
        rect = header.rectangle()
        point = (rect.left + rect.width() // 2, rect.top + rect.height() // 2)
        try:
            hit = desktop.from_point(*point)
        except Exception as exc:
            failures.append(f"Header {index}: hit-test failed ({exc}).")
            continue
        if hit is None:
            failures.append(f"Header {index}: hit-test returned None.")
            continue
        if not _is_descendant_of(hit, table):
            failures.append(
                f"Header {index}: hit-test returned element outside table "
                f"({hit.element_info.control_type}/{hit.element_info.name})."
            )
    if failures:
        return CheckResult(ok=False, details="; ".join(failures))
    if used_fallback:
        return CheckResult(ok=True, details="Header hit-testing OK (top-row fallback).")
    return CheckResult(ok=True, details="Header hit-testing OK.")


def _find_main_window(
    title_hint: Optional[str], window_index: Optional[int]
) -> Optional[UIAWrapper]:
    return select_window(title_hint, window_index)


def main() -> int:
    parser = argparse.ArgumentParser(description="x64dbg UIA accessibility checks.")
    parser.add_argument(
        "--window-title",
        default=None,
        help="Substring to match the main x64dbg window title (optional).",
    )
    parser.add_argument(
        "--window-index",
        type=int,
        default=None,
        help="Select a specific matching window by index (0-based).",
    )
    parser.add_argument(
        "--list-windows",
        action="store_true",
        help="List matching windows and exit.",
    )
    parser.add_argument(
        "--table-name",
        default=None,
        help="Substring to match the UIA Table name (optional).",
    )
    args = parser.parse_args()

    matches = list_matching_windows(args.window_title)
    windows = [match.window for match in matches]
    if args.list_windows:
        if not windows:
            print("No matching windows found.", file=sys.stderr)
            return 1
        for match in matches:
            print(f"{match.index}\t{match.title}")
        return 0

    if not windows:
        print("ERROR: Could not find a window to attach to.", file=sys.stderr)
        return 2

    if args.window_index is not None and not (0 <= args.window_index < len(windows)):
        print("ERROR: --window-index is out of range.", file=sys.stderr)
        return 2

    window = _find_main_window(args.window_title, args.window_index)
    if window is None:
        print("ERROR: Could not find a window to attach to.", file=sys.stderr)
        return 2

    table = _find_table(window, args.table_name)
    if table is None:
        print(
            "ERROR: Could not find a Table control under the window.", file=sys.stderr
        )
        return 3

    print(f"Window: {window.window_text()}")
    print(f"Table: {table.element_info.name or '<unnamed>'}")

    checks = [
        ("child-count", _check_child_count(table)),
        ("header-hit-test", _check_header_hit_test(table)),
    ]

    failed = False
    for name, result in checks:
        status = "OK" if result.ok else "FAIL"
        print(f"[{status}] {name}: {result.details}")
        failed = failed or not result.ok

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
