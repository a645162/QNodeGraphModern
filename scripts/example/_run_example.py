"""Shared launcher for the QNodeGraph example applications."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from _qnodegraph import (  # noqa: E402
    ROOT,
    build_environment,
    configure_command,
    find_executable,
    resolve_qt_prefix,
)


def parse_args(example_name: str) -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(description=f"Build and run {example_name}.")
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    parser.add_argument("--no-build", action="store_true", help="Run the existing executable without building.")
    return parser.parse_known_args()


def run(example_name: str) -> int:
    args, example_args = parse_args(example_name)
    build_dir = args.build_dir or ROOT / "build" / args.configuration
    qt_prefix = resolve_qt_prefix(args.qt_prefix)
    environment = build_environment(qt_prefix)

    if not args.no_build:
        if not (build_dir / "CMakeCache.txt").is_file():
            subprocess.run(
                configure_command(build_dir, args.configuration, qt_prefix),
                cwd=ROOT,
                env=environment,
                check=True,
            )
        subprocess.run(
            ["cmake", "--build", str(build_dir), "--config", args.configuration, "--parallel", "--target", example_name],
            cwd=ROOT,
            env=environment,
            check=True,
        )

    executable = find_executable(build_dir, example_name)
    return subprocess.run(
        [str(executable), *example_args],
        cwd=executable.parent,
        env=environment,
    ).returncode


def example_name_from_script() -> str:
    stem = Path(sys.argv[0]).stem
    if stem.startswith("run_"):
        stem = stem[len("run_"):]
    return "QNodeGraph." + "".join(part.capitalize() for part in stem.split("_"))


if __name__ == "__main__":
    raise SystemExit(run(example_name_from_script()))
