"""Install the project and deploy Qt runtime files for both examples."""

from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path

from _qnodegraph import ROOT, build_environment, find_deploy_tool, find_executable, resolve_qt_prefix


EXAMPLES = ("QNodeGraph.BasicDemo", "QNodeGraph.ImagePipelineDemo")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configuration", choices=("Debug", "Release"), default="Release")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--staging-dir", type=Path)
    parser.add_argument("--qt-prefix", type=Path)
    args = parser.parse_args()
    build_dir = args.build_dir or ROOT / "build" / args.configuration
    staging_dir = args.staging_dir or ROOT / "build" / "package" / args.configuration
    qt_prefix = resolve_qt_prefix(args.qt_prefix)
    environment = build_environment(qt_prefix)
    deploy_tool = find_deploy_tool(qt_prefix)

    subprocess.run(
        ["cmake", "--install", str(build_dir), "--config", args.configuration, "--prefix", str(staging_dir)],
        cwd=ROOT,
        env=environment,
        check=True,
    )
    for name in EXAMPLES:
        executable = find_executable(build_dir, name)
        target = staging_dir / f"{name}.exe"
        shutil.copy2(executable, target)
        subprocess.run(
            [str(deploy_tool), "--no-translations", "--qmldir", str(ROOT / "examples"), str(target)],
            cwd=ROOT,
            env=environment,
            check=True,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
