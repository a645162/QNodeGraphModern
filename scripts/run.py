"""Build and run one of the QNodeGraph examples."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("demo", choices=("basic", "image-pipeline"), default="basic", nargs="?")
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    parser.add_argument("--no-build", action="store_true")
    args, example_args = parser.parse_known_args()
    launcher = ROOT / "scripts" / "example" / (
        "run_basic_demo.py" if args.demo == "basic" else "run_image_pipeline_demo.py"
    )
    command = [sys.executable, str(launcher), "--configuration", args.configuration]
    if args.build_dir:
        command.extend(("--build-dir", str(args.build_dir)))
    if args.qt_prefix:
        command.extend(("--qt-prefix", str(args.qt_prefix)))
    if args.no_build:
        command.append("--no-build")
    command.extend(example_args)
    return subprocess.run(command, cwd=ROOT).returncode


if __name__ == "__main__":
    raise SystemExit(main())
