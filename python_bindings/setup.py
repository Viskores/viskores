##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext


ROOT = Path(__file__).resolve().parent
REPO_ROOT = ROOT.parent


def read_version() -> str:
    return REPO_ROOT.joinpath("version.txt").read_text(encoding="utf-8").strip()


class CMakeBuildExt(build_ext):
    def run(self) -> None:
        for extension in self.extensions:
            self.build_extension(extension)

    def build_extension(self, extension: Extension) -> None:
        ext_fullpath = Path(self.get_ext_fullpath(extension.name)).resolve()
        extdir = ext_fullpath.parent
        build_temp = Path(self.build_temp).resolve() / extension.name
        cache_file = build_temp / "CMakeCache.txt"
        if cache_file.exists():
            cache_text = cache_file.read_text(encoding="utf-8", errors="ignore")
            expected_source = str(ROOT / "wheel")
            if expected_source not in cache_text:
                shutil.rmtree(build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)
        extdir.mkdir(parents=True, exist_ok=True)

        config = "Debug" if self.debug else "Release"
        cmake_args = [
            "cmake",
            "-S",
            str(ROOT / "wheel"),
            "-B",
            str(build_temp),
            f"-DCMAKE_BUILD_TYPE={config}",
            f"-DPython_EXECUTABLE={sys.executable}",
            f"-DVISKORES_PYTHON_EXTENSION_OUTPUT_DIR={extdir}",
            f"-DVISKORES_PYTHON_EXTENSION_FILENAME={ext_fullpath.name}",
        ]

        viskores_dir = os.environ.get("Viskores_DIR") or os.environ.get("VISKORES_DIR")
        if viskores_dir:
            cmake_args.append(f"-DViskores_DIR={viskores_dir}")

        openmp_root = os.environ.get("OpenMP_ROOT") or os.environ.get("OPENMP_ROOT")
        if openmp_root:
            cmake_args.append(f"-DOpenMP_ROOT={openmp_root}")
        else:
            default_openmp_root = Path("/opt/homebrew/opt/libomp")
            if sys.platform == "darwin" and default_openmp_root.exists():
                cmake_args.append(f"-DOpenMP_ROOT={default_openmp_root}")

        subprocess.check_call(cmake_args, cwd=ROOT)

        build_args = ["cmake", "--build", str(build_temp), "--config", config]
        if self.parallel:
            build_args.extend(["--parallel", str(self.parallel)])
        subprocess.check_call(build_args, cwd=ROOT)


setup(
    name="viskores",
    version=read_version(),
    description="Python bindings for Viskores",
    packages=["viskores"],
    package_dir={"": "."},
    ext_modules=[Extension("viskores._viskores", sources=[])],
    cmdclass={"build_ext": CMakeBuildExt},
    install_requires=["numpy"],
    zip_safe=False,
)
