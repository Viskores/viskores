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

from viskores.filter.field_transform import PointTransform
from viskores.filter.vector_analysis import VectorMagnitude
from viskores.io import VTKDataSetReader
from viskores.rendering import MapperRayTracer

from render_test_utils import render_with_mapper


def main():
    repo_root = Path(__file__).resolve().parents[2]
    dataset_path = repo_root / "data" / "data" / "unstructured" / "PointTransformTestDataSet.vtk"

    reader = VTKDataSetReader(str(dataset_path))
    dataset = reader.ReadDataSet()

    point_transform = PointTransform()
    point_transform.SetOutputFieldName("translation")
    point_transform.SetTranslation((1.0, 1.0, 1.0))
    result = point_transform.Execute(dataset)

    vector_magnitude = VectorMagnitude()
    vector_magnitude.SetActiveField("translation")
    vector_magnitude.SetOutputFieldName("pointvar")
    result = vector_magnitude.Execute(result)

    render_with_mapper(result, "pointvar", MapperRayTracer(), "point_transform.png")


if __name__ == "__main__":
    main()
