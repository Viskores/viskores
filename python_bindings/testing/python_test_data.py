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

from viskores.testing import MakeTestDataSet


POINTVAR_3D_UNIFORM_DATASET0 = np.array(
    [
        10.1,
        20.1,
        30.1,
        40.1,
        50.2,
        60.2,
        70.2,
        80.2,
        90.3,
        100.3,
        110.3,
        120.3,
        130.4,
        140.4,
        150.4,
        160.4,
        170.5,
        180.5,
    ],
    dtype=np.float64,
)


def make_test_data_set():
    return MakeTestDataSet()


def make_3d_uniform_dataset0():
    return make_test_data_set().Make3DUniformDataSet0()


def make_vec_pointvar_dataset0():
    dataset = make_3d_uniform_dataset0()
    vectors = np.stack(
        [POINTVAR_3D_UNIFORM_DATASET0, POINTVAR_3D_UNIFORM_DATASET0, POINTVAR_3D_UNIFORM_DATASET0],
        axis=1,
    )
    dataset.AddPointField("vec_pointvar", vectors)
    return dataset
