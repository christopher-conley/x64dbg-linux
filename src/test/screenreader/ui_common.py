from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Optional

from pywinauto import Desktop
from pywinauto.controls.uiawrapper import UIAWrapper


@dataclass
class WindowMatch:
    index: int
    title: str
    window: UIAWrapper


def iter_descendants(element: UIAWrapper) -> Iterable[UIAWrapper]:
    try:
        for child in element.children():
            yield child
            yield from iter_descendants(child)
    except Exception:
        return


def supports_grid(element: UIAWrapper) -> bool:
    try:
        _ = element.iface_grid
        return True
    except Exception:
        return False


def list_matching_windows(title_hint: Optional[str]) -> list[WindowMatch]:
    desktop = Desktop(backend="uia")
    windows = desktop.windows()
    patterns = (
        [title_hint] if title_hint else ["- x64dbg", "- x32dbg", "x64dbg", "x32dbg"]
    )
    matched = []
    for pattern in patterns:
        title_hint_lower = pattern.lower()
        matched = [
            window
            for window in windows
            if title_hint_lower in (window.window_text() or "").lower()
        ]
        if matched:
            break
    windows = matched
    matches = []
    for index, window in enumerate(windows):
        matches.append(
            WindowMatch(index=index, title=window.window_text(), window=window)
        )
    return matches


def select_window(
    title_hint: Optional[str],
    window_index: Optional[int],
) -> Optional[UIAWrapper]:
    matches = list_matching_windows(title_hint)
    if not matches:
        return None
    if window_index is not None:
        if 0 <= window_index < len(matches):
            return matches[window_index].window
        return None
    if len(matches) == 1:
        return matches[0].window

    best_window = None
    best_score = -1
    for match in matches:
        score = 0
        count = 0
        for element in iter_descendants(match.window):
            count += 1
            if element.element_info.control_type in {"Table", "DataGrid"}:
                score += 10
            if supports_grid(element):
                score += 10
            if element.element_info.control_type == "List":
                score += 1
        if score > best_score or (score == best_score and count > 0):
            best_score = score
            best_window = match.window
    return best_window or matches[0].window


def find_tables(root: UIAWrapper, name_hint: Optional[str]) -> list[UIAWrapper]:
    tables = []
    for element in iter_descendants(root):
        if element.element_info.control_type in {"Table", "DataGrid"} or supports_grid(
            element
        ):
            tables.append(element)
    if not tables:
        return []
    if name_hint:
        name_hint_lower = name_hint.lower()
        return [
            table
            for table in tables
            if name_hint_lower in (table.element_info.name or "").lower()
        ]
    return tables


def find_lists(root: UIAWrapper, name_hint: Optional[str]) -> list[UIAWrapper]:
    lists = []
    for element in iter_descendants(root):
        if element.element_info.control_type == "List":
            lists.append(element)
    if not lists:
        return []
    if name_hint:
        name_hint_lower = name_hint.lower()
        return [
            lst
            for lst in lists
            if name_hint_lower in (lst.element_info.name or "").lower()
        ]
    return lists
