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


def check_roundtrip(dataset, output_path, use_binary):
    writer = VTKDataSetWriter(str(output_path))
    if use_binary:
        writer.SetFileTypeToBinary()
    else:
        writer.SetFileTypeToAscii()
    writer.WriteDataSet(dataset)

    reader = VTKDataSetReader(str(output_path))
    read_back = reader.ReadDataSet()

    assert read_back.GetNumberOfPoints() == dataset.GetNumberOfPoints()
    assert read_back.GetNumberOfCells() == dataset.GetNumberOfCells()
    for field_name in ("pointvar", "cellvar"):
        assert read_back.HasField(field_name)
        assert np.allclose(read_back.GetField(field_name), dataset.GetField(field_name))


def main():
    dataset = MakeTestDataSet().Make3DExplicitDataSet5()
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_dir_path = Path(temp_dir)
        check_roundtrip(dataset, temp_dir_path / "explicit_ascii.vtk", use_binary=False)
        check_roundtrip(dataset, temp_dir_path / "explicit_binary.vtk", use_binary=True)


if __name__ == "__main__":
    main()
