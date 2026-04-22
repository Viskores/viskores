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

from viskores.filter.geometry_refinement import Triangulate
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    triangulate = Triangulate()
    triangulate.SetFieldsToPass(("pointvar", "cellvar"))
    output = triangulate.Execute(maker.Make2DUniformDataSet1())

    assert output.GetNumberOfCells() == 32
    assert output.GetField("pointvar").shape == (25,)

    values = output.GetField("cellvar")
    assert np.isclose(values[2], 1.0)
    assert np.isclose(values[3], 1.0)
    assert np.isclose(values[30], 15.0)
    assert np.isclose(values[31], 15.0)

    triangulate = Triangulate()
    triangulate.SetFieldsToPass(("pointvar", "cellvar"))
    output = triangulate.Execute(maker.Make2DExplicitDataSet0())

    assert output.GetNumberOfCells() == 14
    assert output.GetField("pointvar").shape == (16,)

    values = output.GetField("cellvar")
    assert np.isclose(values[1], 1.0)
    assert np.isclose(values[2], 1.0)
    assert np.isclose(values[5], 3.0)
    assert np.isclose(values[6], 3.0)


if __name__ == "__main__":
    main()
