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

from viskores.io import VTKDataSetReader, VTKDataSetWriter
from viskores.testing import MakeTestDataSet


def main():
    repo_root = Path(__file__).resolve().parents[2]
    script = repo_root / "python_bindings/examples/tetrahedra/Tetrahedralize.py"

    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        input_path = temp_path / "input.vtk"
        output_path = temp_path / "out_tets.vtk"

        dataset = MakeTestDataSet().Make3DExplicitDataSet5()
        VTKDataSetWriter(str(input_path)).WriteDataSet(dataset)

        env = os.environ.copy()
        pythonpath = env.get("PYTHONPATH", "")
        env["PYTHONPATH"] = pythonpath if pythonpath else str(repo_root / "viskores-build/python_bindings")

        subprocess.run(
            [sys.executable, str(script), str(input_path), str(output_path)],
            check=True,
            cwd=temp_path,
            env=env,
        )

        assert output_path.is_file()
        output = VTKDataSetReader(str(output_path)).ReadDataSet()
        assert output.GetNumberOfCells() > 0


if __name__ == "__main__":
    main()
