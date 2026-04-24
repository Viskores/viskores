##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    repo_root = Path(__file__).resolve().parents[2]
    script = (
        repo_root
        / "python_bindings/examples/polyline_archimedean_helix/PolyLineArchimedeanHelix.py"
    )

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)

        env = os.environ.copy()
        pythonpath = env.get("PYTHONPATH", "")
        env["PYTHONPATH"] = pythonpath if pythonpath else str(repo_root / "viskores-build/python_bindings")

        subprocess.run(
            [sys.executable, str(script)],
            check=True,
            cwd=temp_path,
            env=env,
        )

        for path in (
            temp_path / "tube_output_50_sides.pnm",
            temp_path / "tube_output_50_sides.png",
            temp_path / "tube_output_4_sides.pnm",
            temp_path / "tube_output_4_sides.png",
        ):
            assert path.is_file()
            assert path.stat().st_size > 0


if __name__ == "__main__":
    main()
