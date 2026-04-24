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

from viskores import Box, Sphere
from viskores.filter.entity_extraction import ExtractPoints
from viskores.testing import MakeTestDataSet


def test_uniform_by_box_inside():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    extract = ExtractPoints()
    extract.SetImplicitFunction(Box((1.0, 1.0, 1.0), (3.0, 3.0, 3.0)))
    extract.SetExtractInside(True)
    extract.SetCompactPoints(True)

    output = extract.Execute(dataset)
    assert output.GetNumberOfCells() == 27
    point_field = output.GetField("pointvar")
    assert point_field.shape[0] == output.GetNumberOfPoints()
    assert np.isclose(point_field[0], 99.0)
    assert np.isclose(point_field[26], 97.0)


def test_uniform_by_box_outside():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    extract = ExtractPoints()
    extract.SetImplicitFunction(Box((1.0, 1.0, 1.0), (3.0, 3.0, 3.0)))
    extract.SetExtractInside(False)
    extract.SetCompactPoints(True)

    output = extract.Execute(dataset)
    assert output.GetNumberOfCells() == 98
    point_field = output.GetField("pointvar")
    assert point_field.shape[0] == output.GetNumberOfPoints()
    assert np.allclose(point_field, 0.0)


def test_uniform_by_sphere():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    extract = ExtractPoints()
    extract.SetImplicitFunction(Sphere((2.0, 2.0, 2.0), 1.8))
    extract.SetExtractInside(True)

    output = extract.Execute(dataset)
    assert output.GetNumberOfCells() == 27


def test_explicit_by_box():
    dataset = MakeTestDataSet().Make3DExplicitDataSet5()
    box = Box((0.0, 0.0, 0.0), (1.0, 1.0, 1.0))

    extract = ExtractPoints()
    extract.SetImplicitFunction(box)
    extract.SetExtractInside(True)
    inside = extract.Execute(dataset)
    assert inside.GetNumberOfCells() == 8

    extract = ExtractPoints()
    extract.SetImplicitFunction(box)
    extract.SetExtractInside(False)
    outside = extract.Execute(dataset)
    assert outside.GetNumberOfCells() == 3


def main():
    test_uniform_by_box_inside()
    test_uniform_by_box_outside()
    test_uniform_by_sphere()
    test_explicit_by_box()


if __name__ == "__main__":
    main()
