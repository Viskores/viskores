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

from render_test_utils import render_with_mapper
from viskores.filter.geometry_refinement import SplitSharpEdges
from viskores.io import VTKDataSetReader
from viskores.rendering import MapperRayTracer


def main():
    repo_root = Path(__file__).resolve().parents[2]
    dataset_path = repo_root / "data" / "data" / "unstructured" / "SplitSharpEdgesTestDataSet.vtk"

    dataset = VTKDataSetReader(str(dataset_path)).ReadDataSet()

    split = SplitSharpEdges()
    split.SetFeatureAngle(89.0)
    split.SetActiveField("Normals", association="cells")
    result = split.Execute(dataset)

    assert result.GetNumberOfCells() == dataset.GetNumberOfCells()
    assert result.GetNumberOfPoints() >= dataset.GetNumberOfPoints()
    assert result.HasField("pointvar", association="points")

    render_with_mapper(result, "pointvar", MapperRayTracer(), "split_sharp_edges.png")


if __name__ == "__main__":
    main()
