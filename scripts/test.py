"""Build and run the QNodeGraph CTest suite."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--build-dir", type=Path)
    args = parser.parse_args()
    build_dir = args.build_dir or ROOT / "build" / args.configuration
    subprocess.run(["cmake", "--build", str(build_dir), "--config", args.configuration, "--parallel"], cwd=ROOT, check=True)
    return subprocess.run(["ctest", "--test-dir", str(build_dir), "--build-config", args.configuration, "--output-on-failure"], cwd=ROOT).returncode


if __name__ == "__main__":
    raise SystemExit(main())
