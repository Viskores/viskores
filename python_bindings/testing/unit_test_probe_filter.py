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

import viskores.cont
from viskores import Association
from viskores.cont import create_uniform_dataset
from viskores.filter.clean_grid import CleanGrid
from viskores.filter.resampling import Probe


def make_input_dataset():
    point_values = np.arange(16, dtype=np.float32) * 0.3
    cell_values = np.arange(9, dtype=np.float32) * 0.7
    dataset = create_uniform_dataset((4, 4), origin=(0.0, 0.0), spacing=(1.0, 1.0))
    dataset.AddPointField("pointdata", point_values)
    dataset.AddCellField("celldata", cell_values)
    return dataset


def make_geometry_dataset():
    return create_uniform_dataset((9, 9), origin=(0.7, 0.7), spacing=(0.35, 0.35))


def convert_uniform_to_explicit(dataset):
    clean = CleanGrid()
    clean.SetMergePoints(True)
    return clean.Execute(dataset)


EXPECTED_POINT_DATA = np.array(
    [
        1.05,
        1.155,
        1.26,
        1.365,
        1.47,
        1.575,
        1.68,
        np.nan,
        np.nan,
        1.47,
        1.575,
        1.68,
        1.785,
        1.89,
        1.995,
        2.1,
        np.nan,
        np.nan,
        1.89,
        1.995,
        2.1,
        2.205,
        2.31,
        2.415,
        2.52,
        np.nan,
        np.nan,
        2.31,
        2.415,
        2.52,
        2.625,
        2.73,
        2.835,
        2.94,
        np.nan,
        np.nan,
        2.73,
        2.835,
        2.94,
        3.045,
        3.15,
        3.255,
        3.36,
        np.nan,
        np.nan,
        3.15,
        3.255,
        3.36,
        3.465,
        3.57,
        3.675,
        3.78,
        np.nan,
        np.nan,
        3.57,
        3.675,
        3.78,
        3.885,
        3.99,
        4.095,
        4.2,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
    ],
    dtype=np.float32,
)

EXPECTED_CELL_DATA = np.array(
    [
        0.0,
        0.7,
        0.7,
        0.7,
        1.4,
        1.4,
        1.4,
        np.nan,
        np.nan,
        2.1,
        2.8,
        2.8,
        2.8,
        3.5,
        3.5,
        3.5,
        np.nan,
        np.nan,
        2.1,
        2.8,
        2.8,
        2.8,
        3.5,
        3.5,
        3.5,
        np.nan,
        np.nan,
        2.1,
        2.8,
        2.8,
        2.8,
        3.5,
        3.5,
        3.5,
        np.nan,
        np.nan,
        4.2,
        4.9,
        4.9,
        4.9,
        5.6,
        5.6,
        5.6,
        np.nan,
        np.nan,
        4.2,
        4.9,
        4.9,
        4.9,
        5.6,
        5.6,
        5.6,
        np.nan,
        np.nan,
        4.2,
        4.9,
        4.9,
        4.9,
        5.6,
        5.6,
        5.6,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
        np.nan,
    ],
    dtype=np.float32,
)

EXPECTED_HIDDEN_POINTS = np.array(
    [
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
    ],
    dtype=np.uint8,
)

EXPECTED_HIDDEN_CELLS = np.array(
    [
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
        2,
    ],
    dtype=np.uint8,
)


def assert_array_equal_with_nan(actual, expected):
    assert actual.shape == expected.shape
    assert np.array_equal(np.isnan(actual), np.isnan(expected))
    mask = ~np.isnan(expected)
    np.testing.assert_allclose(actual[mask], expected[mask], rtol=1e-6, atol=1e-6)


def check_probe_output(output, expected_hidden_cells):
    assert_array_equal_with_nan(output.GetField("pointdata"), EXPECTED_POINT_DATA)
    assert_array_equal_with_nan(output.GetField("celldata"), EXPECTED_CELL_DATA)
    np.testing.assert_array_equal(output.GetField("HIDDEN", Association.POINTS), EXPECTED_HIDDEN_POINTS)
    np.testing.assert_array_equal(output.GetField("HIDDEN", Association.CELLS), expected_hidden_cells)


def run_probe(input_dataset, geometry_dataset):
    probe = Probe()
    probe.SetGeometry(geometry_dataset)
    probe.SetFieldsToPass(["pointdata", "celldata"])
    return probe.Execute(input_dataset)


def run_probe_with_point_geometry(input_dataset, geometry_dataset):
    probe = Probe()
    probe.SetGeometry(geometry_dataset.GetCoordinateSystem().GetData().AsNumPy())
    probe.SetFieldsToPass(["pointdata", "celldata"])
    return probe.Execute(input_dataset)


def test_explicit_to_uniform():
    input_dataset = convert_uniform_to_explicit(make_input_dataset())
    geometry_dataset = make_geometry_dataset()
    check_probe_output(run_probe(input_dataset, geometry_dataset), EXPECTED_HIDDEN_CELLS)
    check_probe_output(run_probe_with_point_geometry(input_dataset, convert_uniform_to_explicit(geometry_dataset)),
                       EXPECTED_HIDDEN_POINTS)


def test_uniform_to_explicit():
    input_dataset = make_input_dataset()
    geometry_dataset = convert_uniform_to_explicit(make_geometry_dataset())
    check_probe_output(run_probe(input_dataset, geometry_dataset), EXPECTED_HIDDEN_CELLS)
    check_probe_output(run_probe_with_point_geometry(input_dataset, geometry_dataset), EXPECTED_HIDDEN_POINTS)


def test_explicit_to_explicit():
    input_dataset = convert_uniform_to_explicit(make_input_dataset())
    geometry_dataset = convert_uniform_to_explicit(make_geometry_dataset())
    check_probe_output(run_probe(input_dataset, geometry_dataset), EXPECTED_HIDDEN_CELLS)
    check_probe_output(run_probe_with_point_geometry(input_dataset, geometry_dataset), EXPECTED_HIDDEN_POINTS)


def main():
    viskores.cont.Initialize(["unit_test_probe_filter.py"])
    test_explicit_to_uniform()
    test_uniform_to_explicit()
    test_explicit_to_explicit()


if __name__ == "__main__":
    main()
