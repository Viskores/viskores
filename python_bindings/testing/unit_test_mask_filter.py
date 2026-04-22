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

from viskores.filter.entity_extraction import Mask
from viskores.testing import MakeTestDataSet


def test_uniform_2d():
    dataset = MakeTestDataSet().Make2DUniformDataSet1()
    mask = Mask()
    mask.SetStride(2)
    output = mask.Execute(dataset)

    assert output.GetNumberOfCells() == 8
    cell_field = output.GetField("cellvar")
    assert cell_field.shape[0] == 8
    assert np.isclose(cell_field[7], 14.0)


def test_uniform_3d():
    dataset = MakeTestDataSet().Make3DUniformDataSet1()
    mask = Mask()
    mask.SetStride(9)
    output = mask.Execute(dataset)

    assert output.GetNumberOfCells() == 7
    cell_field = output.GetField("cellvar")
    assert cell_field.shape[0] == 7
    assert np.isclose(cell_field[2], 18.0)


def test_explicit():
    dataset = MakeTestDataSet().Make3DExplicitDataSet5()
    mask = Mask()
    mask.SetStride(2)
    output = mask.Execute(dataset)

    assert output.GetNumberOfCells() == 2
    cell_field = output.GetField("cellvar")
    assert cell_field.shape[0] == 2
    assert np.isclose(cell_field[1], 120.2)


def main():
    test_uniform_2d()
    test_uniform_3d()
    test_explicit()


if __name__ == "__main__":
    main()
