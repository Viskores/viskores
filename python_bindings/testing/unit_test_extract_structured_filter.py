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

from viskores.filter.entity_extraction import ExtractStructured
from viskores.testing import MakeTestDataSet


def execute(dataset, voi, sample_rate=(1, 1, 1), include_boundary=False):
    extract = ExtractStructured()
    extract.SetVOI(*voi)
    extract.SetSampleRate(*sample_rate)
    extract.SetIncludeBoundary(include_boundary)
    extract.SetFieldsToPass(("pointvar", "cellvar"))
    return extract.Execute(dataset)


def test_uniform_3d_cases():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()

    output = execute(dataset, (1, 4, 1, 4, 1, 4))
    assert output.GetNumberOfPoints() == 27
    assert output.GetNumberOfCells() == 8
    assert np.isclose(output.GetField("pointvar")[0], 99.0)
    assert np.isclose(output.GetField("pointvar")[26], 97.0)
    assert np.isclose(output.GetField("cellvar")[0], 21.0)
    assert np.isclose(output.GetField("cellvar")[7], 42.0)

    output = execute(dataset, (-1, 8, -1, 8, -1, 8))
    assert output.GetNumberOfPoints() == 125
    assert output.GetNumberOfCells() == 64
    assert np.isclose(output.GetField("pointvar")[31], 99.0)
    assert np.isclose(output.GetField("pointvar")[93], 97.0)
    assert np.isclose(output.GetField("cellvar")[0], 0.0)
    assert np.isclose(output.GetField("cellvar")[63], 63.0)

    output = execute(dataset, (-1, 3, -1, 3, -1, 3))
    assert output.GetNumberOfPoints() == 27
    assert output.GetNumberOfCells() == 8
    assert np.isclose(output.GetField("pointvar")[0], 0.0)
    assert np.isclose(output.GetField("pointvar")[26], 15.0)

    output = execute(dataset, (1, 8, 1, 8, 1, 8))
    assert output.GetNumberOfPoints() == 64
    assert output.GetNumberOfCells() == 27
    assert np.isclose(output.GetField("pointvar")[0], 99.0)
    assert np.isclose(output.GetField("pointvar")[63], 0.0)

    output = execute(dataset, (2, 8, 1, 4, 1, 4))
    assert output.GetNumberOfPoints() == 27
    assert output.GetNumberOfCells() == 8
    assert np.isclose(output.GetField("pointvar")[0], 90.0)
    assert np.isclose(output.GetField("pointvar")[26], 0.0)

    output = execute(dataset, (2, 8, 1, 2, 1, 4))
    assert output.GetNumberOfPoints() == 9
    assert output.GetNumberOfCells() == 4
    assert np.isclose(output.GetField("cellvar")[0], 22.0)
    assert np.isclose(output.GetField("cellvar")[3], 39.0)


def test_uniform_3d_sampling_cases():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()

    output = execute(dataset, (0, 5, 0, 5, 1, 4), (2, 2, 1))
    assert output.GetNumberOfPoints() == 27
    assert output.GetNumberOfCells() == 8
    assert np.isclose(output.GetField("cellvar")[0], 16.0)
    assert np.isclose(output.GetField("cellvar")[3], 26.0)

    output = execute(dataset, (0, 5, 0, 5, 1, 4), (3, 3, 2), include_boundary=False)
    assert output.GetNumberOfPoints() == 8
    assert output.GetNumberOfCells() == 1
    point_field = output.GetField("pointvar")
    assert np.isclose(point_field[0], 0.0)
    assert np.isclose(point_field[3], 99.0)
    assert np.isclose(point_field[4], 0.0)
    assert np.isclose(point_field[7], 97.0)
    assert np.isclose(output.GetField("cellvar")[0], 16.0)

    output = execute(dataset, (0, 5, 0, 5, 1, 4), (3, 3, 2), include_boundary=True)
    assert output.GetNumberOfPoints() == 18
    assert output.GetNumberOfCells() == 4
    point_field = output.GetField("pointvar")
    assert np.isclose(point_field[0], 0.0)
    assert np.isclose(point_field[4], 99.0)
    assert np.isclose(point_field[5], 0.0)
    assert np.isclose(point_field[7], 0.0)
    assert np.isclose(point_field[13], 97.0)
    cell_field = output.GetField("cellvar")
    assert np.allclose(cell_field, np.array([16.0, 19.0, 28.0, 31.0], dtype=cell_field.dtype))


def test_rectilinear_cases():
    rect2d = execute(MakeTestDataSet().Make2DRectilinearDataSet0(), (0, 2, 0, 2, 0, 1))
    assert rect2d.GetNumberOfPoints() == 4
    assert rect2d.GetNumberOfCells() == 1
    assert np.isclose(rect2d.GetField("pointvar")[0], 0.0)
    assert np.isclose(rect2d.GetField("pointvar")[3], 4.0)
    assert np.isclose(rect2d.GetField("cellvar")[0], 0.0)

    rect3d = execute(MakeTestDataSet().Make3DRectilinearDataSet0(), (0, 2, 0, 2, 0, 2))
    assert rect3d.GetNumberOfPoints() == 8
    assert rect3d.GetNumberOfCells() == 1
    assert np.isclose(rect3d.GetField("pointvar")[0], 0.0)
    assert np.isclose(rect3d.GetField("pointvar")[7], 10.0)
    assert np.isclose(rect3d.GetField("cellvar")[0], 0.0)


def main():
    test_uniform_3d_cases()
    test_uniform_3d_sampling_cases()
    test_rectilinear_cases()


if __name__ == "__main__":
    main()
