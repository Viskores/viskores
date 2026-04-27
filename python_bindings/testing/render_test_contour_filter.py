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

import viskores
from render_test_utils import render_with_mapper
from viskores.filter.contour import Contour
from viskores.filter.field_conversion import CellAverage
from viskores.filter.field_transform import PointElevation
from viskores.io import VTKDataSetReader
from viskores.rendering import MapperCylinder, MapperRayTracer
from viskores.source import Tangle
from viskores.testing import MakeTestDataSet


def test_contour_filter_uniform():
    maker = MakeTestDataSet()
    input_data = maker.Make3DUniformDataSet0()

    contour = Contour()
    contour.SetGenerateNormals(False)
    contour.SetMergeDuplicatePoints(True)
    contour.SetIsoValues((50.0, 100.0, 150.0))
    contour.SetActiveField("pointvar")
    result = contour.Execute(input_data)
    render_with_mapper(result, "pointvar", MapperRayTracer(), "contour_uniform.png")

    input_data = maker.Make3DUniformDataSet2()
    elevation = PointElevation()
    coords = input_data.GetCoordinateSystem().GetData().AsNumPy()
    low_point = tuple(coords.min(axis=0))
    high_point = tuple(coords.max(axis=0))
    elevation.SetLowPoint(low_point)
    elevation.SetHighPoint(high_point)
    elevation.SetRange(0.0, 1.0)
    elevation.SetOutputFieldName("elevation")
    elevation.SetUseCoordinateSystemAsField(True)
    input_data = elevation.Execute(input_data)

    point_to_cell = CellAverage()
    point_to_cell.SetActiveField("elevation")
    input_data = point_to_cell.Execute(input_data)

    contour.SetIsoValues((80.0,))
    result = contour.Execute(input_data)
    render_with_mapper(result, "elevation", MapperRayTracer(), "contour_uniform_cellfield.png")


def test_contour_filter_uniform_boundaries():
    dataset = viskores.create_uniform_dataset(
        (9, 5, 3), origin=(0.0, 0.0, 0.0), spacing=(0.125, 0.25, 0.5)
    )

    elevation = PointElevation()
    elevation.SetLowPoint((1.0, 0.0, 0.0))
    elevation.SetHighPoint((0.0, 1.0, 1.0))
    elevation.SetOutputFieldName("pointvar")
    elevation.SetUseCoordinateSystemAsField(True)
    dataset = elevation.Execute(dataset)

    contour = Contour()
    contour.SetGenerateNormals(True)
    contour.SetMergeDuplicatePoints(True)
    contour.SetIsoValues((0.25, 0.5, 0.75))
    contour.SetActiveField("pointvar")
    result = contour.Execute(dataset)
    render_with_mapper(result, "pointvar", MapperRayTracer(), "contour_uniform_boundaries.png")


def test_contour_filter_tangle():
    tangle = Tangle()
    tangle.SetCellDimensions((4, 4, 4))
    dataset = tangle.Execute()

    contour = Contour()
    contour.SetGenerateNormals(True)
    contour.SetIsoValue(0, 1.0)
    contour.SetActiveField("tangle")
    contour.SetFieldsToPass("tangle")
    result = contour.Execute(dataset)
    render_with_mapper(result, "tangle", MapperRayTracer(), "contour_tangle.png")


def test_contour_filter_wedge():
    repo_root = Path(__file__).resolve().parents[2]
    dataset_path = repo_root / "data" / "data" / "unstructured" / "wedge_cells.vtk"

    reader = VTKDataSetReader(str(dataset_path))
    dataset = reader.ReadDataSet()

    contour = Contour()
    contour.SetIsoValues((-1.0, 0.0, 1.0))
    contour.SetActiveField("gyroid")
    contour.SetFieldsToPass(("gyroid", "cellvar"))
    contour.SetMergeDuplicatePoints(True)
    result = contour.Execute(dataset)
    render_with_mapper(result, "gyroid", MapperRayTracer(), "contour_wedge.png")


def test_contour_filter_poly():
    repo_root = Path(__file__).resolve().parents[2]
    dataset_path = repo_root / "data" / "data" / "unstructured" / "poly_contour_cases.vtk"

    reader = VTKDataSetReader(str(dataset_path))
    dataset = reader.ReadDataSet()

    contour = Contour()
    contour.SetIsoValues((-0.20, -0.12, -0.04, 0.04, 0.12, 0.20))
    contour.SetActiveField("PerlinNoise")
    contour.SetMergeDuplicatePoints(True)
    result = contour.Execute(dataset)

    mapper = MapperCylinder()
    mapper.SetRadius(0.01)
    render_with_mapper(result, "PerlinNoise", mapper, "contour_poly.png")


def main():
    test_contour_filter_uniform()
    test_contour_filter_uniform_boundaries()
    test_contour_filter_tangle()
    test_contour_filter_wedge()
    test_contour_filter_poly()


if __name__ == "__main__":
    main()
