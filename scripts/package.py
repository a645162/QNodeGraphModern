"""Install the project and deploy Qt runtime files for both examples."""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_QT = Path(r"C:\Qt\6.12.0\llvm-mingw_64")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--staging-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    args = parser.parse_args()
    build_dir = args.build_dir or ROOT / "build" / args.configuration
    staging_dir = args.staging_dir or ROOT / "build" / "package" / args.configuration
    qt_prefix = args.qt_prefix or Path(os.environ.get("QNODEGRAPH_QT_PREFIX", DEFAULT_QT))
    deploy_tool = qt_prefix / "bin" / "windeployqt.exe"
    if not deploy_tool.exists():
        found = shutil.which("windeployqt.exe")
        if not found:
            raise FileNotFoundError("windeployqt.exe was not found; pass --qt-prefix or update PATH")
        deploy_tool = Path(found)

    subprocess.run(["cmake", "--install", str(build_dir), "--config", args.configuration, "--prefix", str(staging_dir)], cwd=ROOT, check=True)
    for name in ("QNodeGraph.BasicDemo.exe", "QNodeGraph.ImagePipelineDemo.exe"):
        executable = next(build_dir.rglob(name), None)
        if executable is None:
            raise FileNotFoundError(f"Example executable was not found: {name}")
        target = staging_dir / name
        shutil.copy2(executable, target)
        subprocess.run([str(deploy_tool), "--no-translations", "--qmldir", str(ROOT / "examples"), str(target)], cwd=ROOT, check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
