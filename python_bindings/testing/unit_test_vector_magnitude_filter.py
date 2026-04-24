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

from python_test_data import POINTVAR_3D_UNIFORM_DATASET0, make_vec_pointvar_dataset0
from viskores.filter.vector_analysis import VectorMagnitude


def main():
    dataset = make_vec_pointvar_dataset0()

    filt = VectorMagnitude()
    filt.SetActiveField("vec_pointvar")
    result = filt.Execute(dataset)

    magnitude = result.GetField("magnitude")
    expected = np.sqrt(3.0 * POINTVAR_3D_UNIFORM_DATASET0 * POINTVAR_3D_UNIFORM_DATASET0)
    assert magnitude.shape == expected.shape
    assert np.allclose(magnitude, expected, rtol=1e-5, atol=1e-5)


if __name__ == "__main__":
    main()
