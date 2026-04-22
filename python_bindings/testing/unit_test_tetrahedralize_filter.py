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

from viskores.filter.geometry_refinement import Tetrahedralize
from viskores.testing import MakeTestDataSet


def main():
    maker = MakeTestDataSet()

    tetrahedralize = Tetrahedralize()
    tetrahedralize.SetFieldsToPass(("pointvar", "cellvar"))
    output = tetrahedralize.Execute(maker.Make3DUniformDataSet0())

    assert output.GetNumberOfCells() == 20
    assert output.GetField("pointvar").shape == (18,)

    values = output.GetField("cellvar")
    assert np.isclose(values[5], 100.2)
    assert np.isclose(values[6], 100.2)
    assert np.isclose(values[7], 100.2)
    assert np.isclose(values[8], 100.2)
    assert np.isclose(values[9], 100.2)

    tetrahedralize = Tetrahedralize()
    tetrahedralize.SetFieldsToPass(("pointvar", "cellvar"))
    output = tetrahedralize.Execute(maker.Make3DExplicitDataSet5())

    assert output.GetNumberOfCells() == 11
    assert output.GetField("pointvar").shape == (11,)

    values = output.GetField("cellvar")
    assert np.isclose(values[5], 110.0)
    assert np.isclose(values[6], 110.0)
    assert np.isclose(values[8], 130.5)
    assert np.isclose(values[9], 130.5)
    assert np.isclose(values[10], 130.5)


if __name__ == "__main__":
    main()
