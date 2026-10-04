"""Configure and build the QNodeGraph project."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

from _qnodegraph import ROOT, build_environment, configure_command, resolve_qt_prefix


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    args = parser.parse_args()
    build_dir = args.build_dir or ROOT / "build" / args.configuration
    qt_prefix = resolve_qt_prefix(args.qt_prefix)
    environment = build_environment(qt_prefix)
    subprocess.run(
        configure_command(build_dir, args.configuration, qt_prefix),
        cwd=ROOT,
        env=environment,
        check=True,
    )
    return subprocess.run(
        ["cmake", "--build", str(build_dir), "--config", args.configuration, "--parallel"],
        cwd=ROOT,
        env=environment,
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
