from __future__ import annotations

import argparse
import os
import queue
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path
from typing import Dict

PLUGIN_COMMAND = "flagrepro3808"
PLUGIN_BASENAME = "FlagRepro3808"
TARGET_BASENAME = "Issue3808Target.exe"
RUNTIME_FILES = [
    "headless.exe",
    "x64bridge.dll",
    "x64dbg.dll",
    "DeviceNameResolver.dll",
    "LLVMDemangle.dll",
    "XEDParse.dll",
    "dbghelp.dll",
    "jansson.dll",
    "lz4.dll",
    "asmjit.dll",
    "TitanEngine.dll",
    "loaddll.exe",
    "msvcp140.dll",
    "vcruntime140.dll",
    "vcruntime140_1.dll",
]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the issue #3808 headless reproduction.")
    parser.add_argument("--headless", required=True, help="Path to headless.exe from an x64dbg build.")
    parser.add_argument("--build-dir", help="Build directory for src/test/issue3808.")
    parser.add_argument("--plugin", help="Path to FlagRepro3808.dp32/.dp64.")
    parser.add_argument("--target", help="Path to Issue3808Target.exe.")
    parser.add_argument("--config", default="Release", help="Build configuration for multi-config generators. Default: Release")
    parser.add_argument("--sandbox", help="Optional sandbox directory. Defaults to a temporary directory.")
    parser.add_argument("--log-file", help="Where to write the captured headless log.")
    parser.add_argument("--timeout", type=int, default=90, help="Timeout in seconds. Default: 90")
    parser.add_argument("--expect", choices=["bug", "fixed"], default="bug", help="Expected outcome. Default: bug")
    parser.add_argument("--keep-sandbox", action="store_true", help="Do not delete the sandbox directory.")
    return parser.parse_args()


def resolve_plugin(build_dir: Path, config: str) -> Path:
    candidates = []
    for base in (build_dir / config, build_dir):
        candidates.extend(sorted(base.glob(f"{PLUGIN_BASENAME}.dp*")))
    if len(candidates) != 1:
        raise FileNotFoundError(f"Expected exactly one {PLUGIN_BASENAME}.dp32/.dp64 under {build_dir}, found: {candidates}")
    return candidates[0]


def resolve_target(build_dir: Path, config: str) -> Path:
    candidates = [build_dir / config / TARGET_BASENAME, build_dir / TARGET_BASENAME]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError(f"Could not find {TARGET_BASENAME} under {build_dir}")


def ensure_file(path: Path, description: str) -> Path:
    if not path.exists() or not path.is_file():
        raise FileNotFoundError(f"Missing {description}: {path}")
    return path.resolve()


def prepare_sandbox(headless: Path, plugin: Path, sandbox: Path) -> Path:
    source_dir = headless.parent
    if sandbox.exists():
        shutil.rmtree(sandbox, ignore_errors=True)
    sandbox.mkdir(parents=True, exist_ok=True)
    (sandbox / "plugins").mkdir(exist_ok=True)

    for filename in RUNTIME_FILES:
        source = source_dir / filename
        ensure_file(source, f"runtime file {filename}")
        shutil.copy2(source, sandbox / filename)

    shutil.copy2(plugin, sandbox / "plugins" / plugin.name)
    return sandbox / "headless.exe"


def run_headless(headless: Path, target: Path, timeout: int) -> subprocess.CompletedProcess[str]:
    command = [str(headless), str(target), "-c", PLUGIN_COMMAND]
    process = subprocess.Popen(
        command,
        cwd=headless.parent,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        bufsize=1,
    )

    output_lines = []
    line_queue: queue.Queue[str | None] = queue.Queue()

    def reader() -> None:
        assert process.stdout is not None
        for line in process.stdout:
            line_queue.put(line)
        line_queue.put(None)

    thread = threading.Thread(target=reader, daemon=True)
    thread.start()

    deadline = time.monotonic() + timeout
    saw_result = False

    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            process.kill()
            thread.join(timeout=1)
            stdout = "".join(output_lines)
            raise RuntimeError(f"Timed out after {timeout}s running: {' '.join(command)}\n\n{stdout}")

        try:
            line = line_queue.get(timeout=min(0.5, remaining))
        except queue.Empty:
            if process.poll() is not None:
                break
            continue

        if line is None:
            break

        output_lines.append(line)
        if "[issue3808] RESULT" in line and not saw_result:
            saw_result = True
            if process.stdin is not None:
                process.stdin.write("exit\n")
                process.stdin.flush()
                process.stdin.close()

    try:
        returncode = process.wait(timeout=max(1, int(deadline - time.monotonic())))
    except subprocess.TimeoutExpired:
        process.kill()
        returncode = process.wait(timeout=5)

    thread.join(timeout=1)
    stdout = "".join(output_lines)
    return subprocess.CompletedProcess(command, returncode, stdout, None)


def parse_result(stdout: str) -> Dict[str, str]:
    result_line = ""
    for line in stdout.splitlines():
        if "[issue3808] RESULT" in line:
            result_line = line
    if not result_line:
        raise RuntimeError("Did not find an [issue3808] RESULT line in headless output.")
    return dict(re.findall(r"(\w+)=([^\s]+)", result_line))


def main() -> int:
    if os.name != "nt":
        print("This reproduction is Windows-only.", file=sys.stderr)
        return 2

    args = parse_args()
    headless = ensure_file(Path(args.headless), "headless executable")

    if args.plugin:
        plugin = ensure_file(Path(args.plugin), "repro plugin")
    else:
        if not args.build_dir:
            raise SystemExit("Either --plugin or --build-dir is required.")
        plugin = ensure_file(resolve_plugin(Path(args.build_dir), args.config), "repro plugin")

    if args.target:
        target = ensure_file(Path(args.target), "repro target")
    else:
        if not args.build_dir:
            raise SystemExit("Either --target or --build-dir is required.")
        target = ensure_file(resolve_target(Path(args.build_dir), args.config), "repro target")

    sandbox_path = Path(args.sandbox) if args.sandbox else Path(tempfile.mkdtemp(prefix="issue3808-headless-"))
    sandbox_created = not args.sandbox
    log_file = Path(args.log_file) if args.log_file else sandbox_path / "issue3808.log"
    cleanup_sandbox = sandbox_created and not args.keep_sandbox

    try:
        sandbox_headless = prepare_sandbox(headless, plugin, sandbox_path)
        completed = run_headless(sandbox_headless, target, args.timeout)
        log_file.write_text(completed.stdout, encoding="utf-8", errors="replace")

        result = parse_result(completed.stdout)
        broken = result.get("broken") == "1"
        fixed = (
            not broken
            and result.get("final_expr_zf") == "0"
            and result.get("final_expr_cf") == "1"
            and result.get("final_api_zf") == "0"
            and result.get("final_api_cf") == "1"
        )

        if args.expect == "bug":
            success = broken
            summary = "REPRODUCED" if success else "NOT REPRODUCED"
        else:
            success = fixed
            summary = "FIX VERIFIED" if success else "FIX NOT VERIFIED"

        if not success:
            cleanup_sandbox = False

        sandbox_display = str(sandbox_path)
        log_display = str(log_file)
        if cleanup_sandbox:
            sandbox_display += " (temporary; use --keep-sandbox to preserve)"
            log_display += " (temporary; use --keep-sandbox or --log-file to preserve)"

        print(f"headless: {headless}")
        print(f"plugin:   {plugin}")
        print(f"target:   {target}")
        print(f"sandbox:  {sandbox_display}")
        print(f"log:      {log_display}")
        print(f"result:   {summary}")
        print(f"details:  {result}")

        if completed.returncode != 0:
            print(f"warning: headless exited with code {completed.returncode}", file=sys.stderr)

        return 0 if success else 1
    except Exception:
        cleanup_sandbox = False
        raise
    finally:
        if cleanup_sandbox and sandbox_path.exists():
            shutil.rmtree(sandbox_path, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
