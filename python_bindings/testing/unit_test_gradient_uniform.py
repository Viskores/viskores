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

from python_test_data import make_3d_uniform_dataset0, make_vec_pointvar_dataset0
from viskores.filter.vector_analysis import Gradient


def test_scalar_gradient_failure():
    dataset = make_3d_uniform_dataset0()

    gradient = Gradient()
    gradient.SetOutputFieldName("Gradient")
    gradient.SetComputeVorticity(True)
    gradient.SetComputeQCriterion(True)
    gradient.SetActiveField("pointvar")

    try:
        gradient.Execute(dataset)
    except RuntimeError:
        return
    raise AssertionError("Gradient should fail when vorticity or q-criterion are requested for scalars.")


def test_vector_cell_gradient():
    dataset = make_vec_pointvar_dataset0()

    gradient = Gradient()
    gradient.SetOutputFieldName("vec_gradient")
    gradient.SetComputeDivergence(True)
    gradient.SetComputeVorticity(True)
    gradient.SetComputeQCriterion(True)
    gradient.SetActiveField("vec_pointvar")
    result = gradient.Execute(dataset)

    expected_gradient = np.array(
        [
            [10.025002, 10.025002, 10.025002, 30.075003, 30.075003, 30.075003, 60.125, 60.125, 60.125],
            [10.025002, 10.025002, 10.025002, 30.075, 30.075, 30.075, 60.125, 60.125, 60.125],
            [10.025002, 10.025002, 10.025002, 30.075005, 30.075005, 30.075005, 60.174995, 60.174995, 60.174995],
            [10.025002, 10.025002, 10.025002, 30.075005, 30.075005, 30.075005, 60.175003, 60.175003, 60.175003],
        ],
        dtype=np.float64,
    )
    expected_divergence = np.array([100.225006, 100.225006, 100.275, 100.27501], dtype=np.float64)
    expected_vorticity = np.array(
        [
            [-30.049997, 50.1, -20.050001],
            [-30.05, 50.1, -20.05],
            [-30.09999, 50.149994, -20.050003],
            [-30.099998, 50.15, -20.050003],
        ],
        dtype=np.float64,
    )
    expected_qcriterion = np.array([-5022.5254, -5022.5254, -5027.538, -5027.539], dtype=np.float64)

    assert np.allclose(result.GetField("vec_gradient"), expected_gradient, rtol=1e-5, atol=1e-5)
    assert np.allclose(result.GetField("Divergence"), expected_divergence, rtol=1e-5, atol=1e-5)
    assert np.allclose(result.GetField("Vorticity"), expected_vorticity, rtol=1e-5, atol=1e-5)
    assert np.allclose(result.GetField("QCriterion"), expected_qcriterion, rtol=1e-5, atol=1e-5)


def test_vector_point_gradient():
    dataset = make_vec_pointvar_dataset0()

    gradient = Gradient()
    gradient.SetComputePointGradient(True)
    gradient.SetOutputFieldName("vec_gradient")
    gradient.SetActiveField("vec_pointvar")
    result = gradient.Execute(dataset)

    expected = np.array(
        [
            [10.0, 10.0, 10.0, 29.999998, 29.999998, 29.999998, 60.1, 60.1, 60.1],
            [10.0, 10.0, 10.0, 30.1, 30.1, 30.1, 60.1, 60.1, 60.1],
            [10.0, 10.0, 10.0, 30.1, 30.1, 30.1, 60.200005, 60.200005, 60.200005],
            [10.100002, 10.100002, 10.100002, 29.999998, 29.999998, 29.999998, 60.200005, 60.200005, 60.200005],
        ],
        dtype=np.float64,
    )
    computed = result.GetField("vec_gradient")
    assert computed.shape == (18, 9)
    assert np.allclose(computed[:4], expected, rtol=1e-5, atol=1e-5)


def main():
    test_scalar_gradient_failure()
    test_vector_cell_gradient()
    test_vector_point_gradient()


if __name__ == "__main__":
    main()
