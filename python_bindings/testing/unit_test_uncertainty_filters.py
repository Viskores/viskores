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

from viskores.cont import Field
from viskores.python_convenience import create_uniform_dataset
from viskores.filter.uncertainty import (
    ContourUncertainUniform,
    ContourUncertainUniformMonteCarlo,
    FiberUncertainUniform,
)


def make_uncertain_contour_dataset():
    dataset = create_uniform_dataset((3, 3, 3))
    ensemble_min = np.zeros(27, dtype=np.float32)
    ensemble_max = np.ones(27, dtype=np.float32)
    dataset.AddPointField("ensemble_min", ensemble_min)
    dataset.AddPointField("ensemble_max", ensemble_max)
    return dataset


def assert_uncertainty_output(dataset, names):
    assert dataset.GetNumberOfCells() == 8
    for name in names:
        assert dataset.HasField(name, Field.Association.Cells)
        assert dataset.GetField(name, Field.Association.Cells).GetData().AsNumPy().shape == (8,)


def test_contour_uncertain_uniform():
    dataset = make_uncertain_contour_dataset()
    uncertainty = ContourUncertainUniform()
    uncertainty.SetMinField("ensemble_min")
    uncertainty.SetMaxField("ensemble_max")
    uncertainty.SetIsoValue(0.5)
    uncertainty.SetCrossProbabilityName("cross_probability")
    uncertainty.SetNumberNonzeroProbabilityName("nonzero_probability")
    uncertainty.SetEntropyName("entropy")

    assert uncertainty.GetIsoValue() == 0.5
    assert uncertainty.GetCrossProbabilityName() == "cross_probability"
    assert uncertainty.GetNumberNonzeroProbabilityName() == "nonzero_probability"
    assert uncertainty.GetEntropyName() == "entropy"

    result = uncertainty.Execute(dataset)
    assert_uncertainty_output(
        result,
        ("cross_probability", "nonzero_probability", "entropy"),
    )


def test_contour_uncertain_uniform_monte_carlo():
    dataset = make_uncertain_contour_dataset()
    uncertainty = ContourUncertainUniformMonteCarlo()
    uncertainty.SetMinField("ensemble_min")
    uncertainty.SetMaxField("ensemble_max")
    uncertainty.SetIsoValue(0.5)
    uncertainty.SetNumSample(8)
    uncertainty.SetCrossProbabilityName("cross_probability_mc")
    uncertainty.SetNumberNonzeroProbabilityName("nonzero_probability_mc")
    uncertainty.SetEntropyName("entropy_mc")

    assert uncertainty.GetIsoValue() == 0.5
    assert uncertainty.GetNumSample() == 8
    assert uncertainty.GetCrossProbabilityName() == "cross_probability_mc"
    assert uncertainty.GetNumberNonzeroProbabilityName() == "nonzero_probability_mc"
    assert uncertainty.GetEntropyName() == "entropy_mc"

    result = uncertainty.Execute(dataset)
    assert_uncertainty_output(
        result,
        ("cross_probability_mc", "nonzero_probability_mc", "entropy_mc"),
    )


def test_fiber_uncertain_uniform():
    dataset = create_uniform_dataset((4, 4, 4))
    values = np.linspace(10.0, 30.0, 64, dtype=np.float32)
    dataset.AddPointField("ensemble_min_1", values - 1.0)
    dataset.AddPointField("ensemble_max_1", values + 1.0)
    dataset.AddPointField("ensemble_min_2", values - 2.0)
    dataset.AddPointField("ensemble_max_2", values + 2.0)

    uncertainty = FiberUncertainUniform()
    uncertainty.SetRange1((15.0, 15.0))
    uncertainty.SetRange2(25.0, 25.0)
    uncertainty.SetField1Min("ensemble_min_1")
    uncertainty.SetField1Max("ensemble_max_1")
    uncertainty.SetField2Min("ensemble_min_2")
    uncertainty.SetField2Max("ensemble_max_2")
    uncertainty.SetApproach(FiberUncertainUniform.ApproachEnum.ClosedForm)

    result = uncertainty.Execute(dataset)
    assert result.HasField("ClosedForm", Field.Association.Points)
    assert result.GetField("ClosedForm", Field.Association.Points).GetData().AsNumPy().shape == (64,)


def main():
    test_contour_uncertain_uniform()
    test_contour_uncertain_uniform_monte_carlo()
    test_fiber_uncertain_uniform()


if __name__ == "__main__":
    main()
