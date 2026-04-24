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

from viskores.filter.vector_analysis import Gradient
from viskores.testing import MakeTestDataSet


def test_cell_gradient_explicit():
    dataset = MakeTestDataSet().Make3DExplicitDataSet0()

    gradient = Gradient()
    gradient.SetOutputFieldName("gradient")
    gradient.SetActiveField("pointvar")

    result = gradient.Execute(dataset)
    expected = np.array([[10.0, 10.1, 0.0], [10.0, 10.1, 0.0]], dtype=np.float32)
    np.testing.assert_allclose(result.GetField("gradient"), expected, rtol=1e-6, atol=1e-6)


def test_point_gradient_explicit():
    dataset = MakeTestDataSet().Make3DExplicitDataSet0()

    gradient = Gradient()
    gradient.SetComputePointGradient(True)
    gradient.SetOutputFieldName("gradient")
    gradient.SetActiveField("pointvar")

    result = gradient.Execute(dataset)
    expected = np.array([[10.0, 10.1, 0.0], [10.0, 10.1, 0.0]], dtype=np.float32)
    computed = result.GetField("gradient")
    assert computed.shape[1] == 3
    np.testing.assert_allclose(computed[:2], expected, rtol=1e-6, atol=1e-6)


def main():
    test_cell_gradient_explicit()
    test_point_gradient_explicit()


if __name__ == "__main__":
    main()
