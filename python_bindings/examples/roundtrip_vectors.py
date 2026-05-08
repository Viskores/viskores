##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import numpy as np

import viskores


# Demonstrate 2D NumPy arrays becoming runtime-component Viskores Vec arrays.
cases = [
    np.arange(8, dtype=np.float32).reshape(4, 2),
    np.arange(12, dtype=np.float64).reshape(4, 3),
    np.arange(16, dtype=np.uint8).reshape(4, 4),
    np.arange(20, dtype=np.int32).reshape(4, 5),
]


for values in cases:
    unknown = viskores.cont.array_from_numpy(values)
    result = viskores.cont.asnumpy(unknown)
    np.testing.assert_array_equal(result, values)
    print(f"{values.dtype} shape={values.shape}: {unknown} ->\n{result}")
