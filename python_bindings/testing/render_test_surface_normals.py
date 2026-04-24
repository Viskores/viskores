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

from viskores.cont import ColorTable
from viskores.filter.vector_analysis import SurfaceNormals
from viskores.io import VTKDataSetReader
from viskores.rendering import MapperRayTracer

from render_test_utils import render_with_mapper


def main():
    repo_root = Path(__file__).resolve().parents[2]
    dataset_path = repo_root / "data" / "data" / "unstructured" / "SurfaceNormalsTestDataSet.vtk"

    reader = VTKDataSetReader(str(dataset_path))
    dataset = reader.ReadDataSet()

    surface_normals = SurfaceNormals()
    surface_normals.SetGeneratePointNormals(True)
    surface_normals.SetAutoOrientNormals(True)
    result = surface_normals.Execute(dataset)

    color_table = ColorTable("inferno")
    render_with_mapper(result, "pointvar", MapperRayTracer(), "surface_normals.png")


if __name__ == "__main__":
    main()
