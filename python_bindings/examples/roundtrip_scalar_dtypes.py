##============================================================================
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##============================================================================

import numpy as np

import viskores
import viskores.cont


# Demonstrate NumPy -> UnknownArrayHandle -> NumPy for every supported scalar dtype.
DTYPES = [
    np.int8,
    np.int16,
    np.int32,
    np.int64,
    np.uint8,
    np.uint16,
    np.uint32,
    np.uint64,
    np.float32,
    np.float64,
]


for dtype in DTYPES:
    values = np.arange(6, dtype=dtype)
    unknown = viskores.cont.array_from_numpy(values)
    result = unknown.asnumpy()
    np.testing.assert_array_equal(result, values)
    print(f"{values.dtype}: {unknown} -> {result}")
