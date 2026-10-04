"""Shared helpers for the QNodeGraph Python entry points."""

from __future__ import annotations

import os
import shutil
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_QT_PREFIX = Path(r"C:\Qt\6.12.0\llvm-mingw_64")
DEFAULT_QT_COMPILER = Path(r"C:\Qt\Tools\llvm-mingw2217_64\bin\clang++.exe")


def resolve_qt_prefix(requested: Path | None) -> Path | None:
    """Resolve the Qt kit from an explicit value, the environment, or the default."""
    if requested:
        return requested
    env_prefix = os.environ.get("QNODEGRAPH_QT_PREFIX")
    if env_prefix:
        return Path(env_prefix)
    if (DEFAULT_QT_PREFIX / "lib" / "cmake" / "Qt6").is_dir():
        return DEFAULT_QT_PREFIX
    return None


def build_environment(qt_prefix: Path | None) -> dict[str, str]:
    """Return a copy of the current environment with the Qt bin directory on PATH."""
    environment = os.environ.copy()
    if qt_prefix is not None and (qt_prefix / "bin").is_dir():
        environment["PATH"] = f"{qt_prefix / 'bin'}{os.pathsep}{environment.get('PATH', '')}"
    return environment


def configure_command(build_dir: Path, configuration: str, qt_prefix: Path | None) -> list[str]:
    """Build the CMake configure command for the requested Qt kit."""
    command = [
        "cmake",
        "-S", str(ROOT),
        "-B", str(build_dir),
        "-G", "Ninja",
        f"-DCMAKE_BUILD_TYPE={configuration}",
        "-DQNODEGRAPH_BUILD_TESTS=ON",
        "-DQNODEGRAPH_BUILD_EXAMPLES=ON",
    ]
    if qt_prefix is not None:
        command.append(f"-DCMAKE_PREFIX_PATH={qt_prefix}")
        # Force the llvm-mingw toolchain bundled with the Qt kit; a clang++ from
        # PATH may target the MSVC ABI and fail to link against the MinGW Qt libs.
        if "llvm-mingw" in str(qt_prefix).lower() and DEFAULT_QT_COMPILER.is_file():
            command.append(f"-DCMAKE_CXX_COMPILER={DEFAULT_QT_COMPILER}")
    return command


def ensure_configured(build_dir: Path, configuration: str, qt_prefix: Path | None) -> None:
    """Configure the build directory when it has not been configured yet."""
    if (build_dir / "CMakeCache.txt").is_file():
        return
    subprocess.run(
        configure_command(build_dir, configuration, qt_prefix),
        cwd=ROOT,
        env=build_environment(qt_prefix),
        check=True,
    )


def find_executable(build_dir: Path, name: str) -> Path:
    """Find an example executable below the given build directory."""
    match = next(build_dir.rglob(f"{name}.exe"), None)
    if match is None:
        raise FileNotFoundError(f"Executable was not found below: {build_dir}")
    return match


def find_deploy_tool(qt_prefix: Path | None) -> Path:
    """Locate windeployqt.exe from the Qt kit or the system PATH."""
    if qt_prefix is not None:
        candidate = qt_prefix / "bin" / "windeployqt.exe"
        if candidate.is_file():
            return candidate
    found = shutil.which("windeployqt.exe")
    if found is None:
        raise FileNotFoundError("windeployqt.exe was not found; pass --qt-prefix or update PATH")
    return Path(found)
