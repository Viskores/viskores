##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import tempfile
from pathlib import Path

import numpy as np

from viskores.io import VTKDataSetReader, VTKDataSetWriter
from viskores.testing import MakeTestDataSet


def main():
    dataset = MakeTestDataSet().Make2DUniformDataSet1()
    with tempfile.TemporaryDirectory() as temp_dir:
        file_name = Path(temp_dir) / "uniform_ascii.vtk"
        writer = VTKDataSetWriter(str(file_name))
        writer.SetFileTypeToAscii()
        writer.WriteDataSet(dataset)

        reader = VTKDataSetReader(str(file_name))
        output = reader.ReadDataSet()

        assert output.GetNumberOfPoints() == dataset.GetNumberOfPoints()
        assert output.GetNumberOfCells() == dataset.GetNumberOfCells()
        assert len(output.FieldNames()) == len(dataset.FieldNames())
        assert np.allclose(output.GetField("pointvar"), dataset.GetField("pointvar"))
        assert np.allclose(output.GetField("cellvar"), dataset.GetField("cellvar"))


if __name__ == "__main__":
    main()
