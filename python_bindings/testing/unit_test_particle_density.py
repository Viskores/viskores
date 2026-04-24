##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

import math
import numpy as np

import viskores.cont
from viskores import CELL_SHAPE_VERTEX
from viskores.cont import DataSetBuilderExplicit
from viskores.filter.density_estimate import ParticleDensityCloudInCell, ParticleDensityNearestGridPoint


def make_particle_dataset():
    num_particles = 1000
    position_rng = np.random.default_rng(0xCEED)
    mass_rng = np.random.default_rng(0xD1CE)

    positions = position_rng.random((num_particles, 3), dtype=np.float32)
    dataset = DataSetBuilderExplicit.Create(
        positions,
        [CELL_SHAPE_VERTEX] * num_particles,
        [1] * num_particles,
        list(range(num_particles)),
    )
    dataset.AddCellField("mass", mass_rng.random(num_particles, dtype=np.float32))
    return dataset


def main():
    viskores.cont.Initialize(["unit_test_particle_density.py"])

    dataset = make_particle_dataset()
    mass_sum = float(np.sum(dataset.GetField("mass")))
    mass_count = dataset.GetField("mass").shape[0]

    ngp = ParticleDensityNearestGridPoint()
    ngp.SetDimension((3, 3, 3))
    ngp.SetBounds((0.0, 1.0, 0.0, 1.0, 0.0, 1.0))
    ngp.SetActiveField("mass")
    assert ngp.GetBounds() == (0.0, 1.0, 0.0, 1.0, 0.0, 1.0)
    np.testing.assert_allclose(ngp.GetOrigin(), (0.0, 0.0, 0.0))
    np.testing.assert_allclose(ngp.GetSpacing(), (1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0), rtol=1e-4)

    density = ngp.Execute(dataset)
    density_sum = float(np.sum(density.GetField("density")))
    assert math.isclose(density_sum, mass_sum * 27.0, rel_tol=0.02, abs_tol=0.1)

    ngp.SetComputeNumberDensity(True)
    ngp.SetDivideByVolume(False)
    counts = ngp.Execute(dataset)
    counts_sum = float(np.sum(counts.GetField("density")))
    assert math.isclose(counts_sum, float(mass_count), rel_tol=0.02, abs_tol=0.1)

    cic = ParticleDensityCloudInCell()
    cic.SetDimension((3, 3, 3))
    cic.SetOrigin((0.0, 0.0, 0.0))
    cic.SetSpacing((1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0))
    cic.SetActiveField("mass")
    np.testing.assert_allclose(cic.GetOrigin(), (0.0, 0.0, 0.0))
    np.testing.assert_allclose(cic.GetSpacing(), (1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0), rtol=1e-4)
    assert cic.GetBounds() == (0.0, 1.0, 0.0, 1.0, 0.0, 1.0)

    density = cic.Execute(dataset)
    density_sum = float(np.sum(density.GetField("density")))
    assert math.isclose(density_sum, mass_sum * 27.0, rel_tol=0.02, abs_tol=0.1)

    cic.SetComputeNumberDensity(True)
    cic.SetDivideByVolume(False)
    counts = cic.Execute(dataset)
    counts_sum = float(np.sum(counts.GetField("density")))
    assert math.isclose(counts_sum, float(mass_count), rel_tol=0.02, abs_tol=0.1)


if __name__ == "__main__":
    main()

