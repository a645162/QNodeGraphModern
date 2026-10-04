"""Configure and build the QNodeGraph project."""

from __future__ import annotations

import argparse
import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_QT = Path(r"C:\Qt\6.12.0\llvm-mingw_64")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Debug")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    args = parser.parse_args()
    build_dir = args.build_dir or ROOT / "build" / args.configuration
    qt_prefix = args.qt_prefix or (Path(os.environ["QNODEGRAPH_QT_PREFIX"]) if os.environ.get("QNODEGRAPH_QT_PREFIX") else DEFAULT_QT)
    env = os.environ.copy()
    if (qt_prefix / "bin").is_dir():
        env["PATH"] = f"{qt_prefix / 'bin'}{os.pathsep}{env.get('PATH', '')}"
    configure = ["cmake", "-S", str(ROOT), "-B", str(build_dir), "-G", "Ninja", f"-DCMAKE_BUILD_TYPE={args.configuration}", "-DQNODEGRAPH_BUILD_TESTS=ON", "-DQNODEGRAPH_BUILD_EXAMPLES=ON"]
    if (qt_prefix / "lib" / "cmake" / "Qt6").is_dir():
        configure.append(f"-DCMAKE_PREFIX_PATH={qt_prefix}")
    subprocess.run(configure, cwd=ROOT, env=env, check=True)
    return subprocess.run(["cmake", "--build", str(build_dir), "--parallel"], cwd=ROOT, env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
