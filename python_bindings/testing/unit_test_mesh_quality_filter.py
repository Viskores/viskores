##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import numpy as np

import viskores
from viskores.cont import DataSetBuilderExplicit, DataSetBuilderExplicitIterative
from viskores.filter.mesh_info import CellMetric, MeshQuality


def make_explicit_dataset():
    coords = [
        (0, 0, 0),
        (3, 0, 0),
        (2, 2, 0),
        (4, 0, 0),
        (7, 0, 0),
        (7, 2, 0),
        (6, 2, 0),
        (8, 0, 0),
        (11, 0, 0),
        (9, 2, 0),
        (9, 1, 1),
        (9, 3, 0),
        (11, 3, 0),
        (11, 5, 0),
        (9, 5, 0),
        (10, 4, 1),
        (12, 0, 0),
        (12, 3, 0),
        (12, 2, 1),
        (15, 0, 0),
        (15, 3, 0),
        (15, 1, 1),
        (16, 0, 0),
        (18, 0, 0),
        (18, 2, 0),
        (16, 2, 0),
        (17, 1, 1),
        (19, 1, 1),
        (19, 3, 1),
        (17, 3, 1),
    ]
    shapes = [
        viskores.CELL_SHAPE_TRIANGLE,
        viskores.CELL_SHAPE_QUAD,
        viskores.CELL_SHAPE_TETRA,
        viskores.CELL_SHAPE_PYRAMID,
        viskores.CELL_SHAPE_WEDGE,
        viskores.CELL_SHAPE_HEXAHEDRON,
    ]
    num_indices = [3, 4, 4, 5, 6, 8]
    connectivity = [
        0, 1, 2,
        3, 4, 5, 6,
        7, 8, 9, 10,
        11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21,
        22, 23, 24, 25, 26, 27, 28, 29,
    ]
    return DataSetBuilderExplicit.Create(coords, shapes, num_indices, connectivity, "coordinates")


def make_single_type_dataset():
    builder = DataSetBuilderExplicitIterative()
    for point in ((0, 0, 0), (3, 0, 0), (2, 2, 0), (4, 0, 0)):
        builder.AddPoint(point)
    builder.AddCell(viskores.CELL_SHAPE_TRIANGLE, [0, 1, 2])
    builder.AddCell(viskores.CELL_SHAPE_TRIANGLE, [2, 1, 3])
    return builder.Create()


def check_metric(dataset, metric, output_name, expected):
    mesh_quality = MeshQuality()
    mesh_quality.SetMetric(metric)
    output = mesh_quality.Execute(dataset)
    np.testing.assert_allclose(output.GetField(output_name), np.array(expected, dtype=np.float64), rtol=1e-5, atol=1e-5)


def main():
    explicit_input = make_explicit_dataset()
    single_type_input = make_single_type_dataset()

    check_metric(explicit_input, CellMetric.Volume, "volume", [0, 0, 1, 1.333333333, 4, 4])
    check_metric(explicit_input, CellMetric.Area, "area", [3, 4, 0, 0, 0, 0])
    check_metric(single_type_input, CellMetric.Area, "area", [3, 1])

    check_metric(
        explicit_input,
        CellMetric.AspectRatio,
        "aspectRatio",
        [1.164010, 1.118034, 1.648938, 0, 0, 1.1547],
    )
    check_metric(single_type_input, CellMetric.AspectRatio, "aspectRatio", [1.164010, 2.47582])

    check_metric(explicit_input, CellMetric.MinAngle, "minAngle", [45, 45, -1, -1, -1, -1])
    check_metric(single_type_input, CellMetric.MinAngle, "minAngle", [45, 18.4348])

    check_metric(
        explicit_input,
        CellMetric.MaxAngle,
        "maxAngle",
        [71.56505, 135, -1, -1, -1, -1],
    )
    check_metric(single_type_input, CellMetric.MaxAngle, "maxAngle", [71.56505, 116.565])

    check_metric(
        explicit_input,
        CellMetric.ScaledJacobian,
        "scaledJacobian",
        [0.816497, 0.707107, 0.408248, -2, -2, 0.57735],
    )
    check_metric(single_type_input, CellMetric.ScaledJacobian, "scaledJacobian", [0.816497, 0.365148])

    check_metric(
        explicit_input,
        CellMetric.Shape,
        "shape",
        [0.944755, 0.444444, 0.756394, -1, -1, 0.68723],
    )
    check_metric(single_type_input, CellMetric.Shape, "shape", [0.944755, 0.494872])

    check_metric(
        explicit_input,
        CellMetric.RelativeSizeSquared,
        "relativeSizeSquared",
        [0.151235, 0.085069, 0.337149, -1, -1, 0.185378],
    )
    check_metric(single_type_input, CellMetric.RelativeSizeSquared, "relativeSizeSquared", [0.444444, 0.25])

    check_metric(
        explicit_input,
        CellMetric.ShapeAndSize,
        "shapeAndSize",
        [0.142880, 0.037809, 0.255017, -1, -1, 0.127397],
    )
    check_metric(single_type_input, CellMetric.ShapeAndSize, "shapeAndSize", [0.419891, 0.123718])


if __name__ == "__main__":
    main()
