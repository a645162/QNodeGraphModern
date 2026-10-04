"""Shared launcher for the QNodeGraph example applications."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_QT_PREFIX = Path(r"C:\Qt\6.12.0\llvm-mingw_64")


def parse_args(example_name: str) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=f"Build and run {example_name}.")
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    parser.add_argument("--no-build", action="store_true", help="Run the existing executable without building.")
    parser.add_argument("--", dest="example_args", nargs=argparse.REMAINDER)
    return parser.parse_args()


def resolve_qt_prefix(requested: Path | None) -> Path | None:
    candidates = [requested]
    env_prefix = os.environ.get("QNODEGRAPH_QT_PREFIX")
    if env_prefix:
        candidates.append(Path(env_prefix))
    candidates.append(DEFAULT_QT_PREFIX)
    for candidate in candidates:
        if candidate and (candidate / "bin").is_dir():
            return candidate
    return None


def run(example_name: str) -> int:
    args = parse_args(example_name)
    build_dir = args.build_dir or REPO_ROOT / "build" / args.configuration
    qt_prefix = resolve_qt_prefix(args.qt_prefix)
    environment = os.environ.copy()
    if qt_prefix:
        environment["PATH"] = f"{qt_prefix / 'bin'}{os.pathsep}{environment.get('PATH', '')}"

    if not args.no_build:
        command = [
            "cmake",
            "--build",
            str(build_dir),
            "--config",
            args.configuration,
            "--parallel",
            "--target",
            example_name,
        ]
        subprocess.run(command, cwd=REPO_ROOT, env=environment, check=True)

    matches = list(build_dir.rglob(f"{example_name}.exe"))
    if not matches:
        raise FileNotFoundError(f"Executable was not found below: {build_dir}")
    return subprocess.run([str(matches[0]), *(args.example_args or [])], cwd=matches[0].parent, env=environment).returncode


if __name__ == "__main__":
    raise SystemExit(run(Path(sys.argv[0]).stem.replace("run_", "QNodeGraph.").replace("_", "")))
