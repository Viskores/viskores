##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

from pathlib import Path

from viskores.filter.contour import Contour
from viskores.filter.field_conversion import CellAverage
from viskores.filter.geometry_refinement import SplitSharpEdges
from viskores.io import VTKDataSetReader
from viskores.source import Wavelet


def test_explicit_data():
    repo_root = Path(__file__).resolve().parents[2]
    dataset_path = repo_root / "data" / "data" / "unstructured" / "SplitSharpEdgesTestDataSet.vtk"
    dataset = VTKDataSetReader(str(dataset_path)).ReadDataSet()

    split = SplitSharpEdges()
    split.SetActiveField("Normals", association="cells")

    split.SetFeatureAngle(89.0)
    split_every_edge = split.Execute(dataset)
    assert split_every_edge.GetNumberOfCells() == 6
    assert split_every_edge.GetNumberOfPoints() == 24
    assert split_every_edge.HasField("pointvar", association="points")
    assert split_every_edge.HasField("cellvar", association="cells")

    split.SetFeatureAngle(91.0)
    no_split = split.Execute(dataset)
    assert no_split.GetNumberOfCells() == 6
    assert no_split.GetNumberOfPoints() == 8
    assert no_split.HasField("pointvar", association="points")
    assert no_split.HasField("cellvar", association="cells")


def test_structured_data():
    wavelet = Wavelet()
    wavelet.SetExtent((-25, -25, -25), (25, 25, 25))
    wavelet.SetFrequency((60, 30, 40))
    wavelet.SetMagnitude((5, 5, 5))
    dataset = wavelet.Execute()

    contour = Contour()
    contour.SetActiveField("RTData", association="points")
    contour.SetIsoValue(192)
    contour.SetMergeDuplicatePoints(True)
    contour.SetGenerateNormals(True)
    contour.SetComputeFastNormals(True)
    dataset = contour.Execute(dataset)

    cell_normals = CellAverage()
    cell_normals.SetActiveField("normals", association="points")
    dataset = cell_normals.Execute(dataset)

    split = SplitSharpEdges()
    split.SetActiveField("normals", association="cells")
    result = split.Execute(dataset)

    assert result.GetNumberOfCells() == dataset.GetNumberOfCells()
    assert result.GetNumberOfPoints() >= dataset.GetNumberOfPoints()
    assert result.HasField("normals", association="points")
    assert result.HasField("normals", association="cells")


def main():
    test_explicit_data()
    test_structured_data()


if __name__ == "__main__":
    main()
