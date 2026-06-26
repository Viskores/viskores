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


# Demonstrate the allow_copy=True kwarg on viskores.cont.array_from_numpy.
# By default the binding requires a directly-shareable input (C-contiguous,
# aligned, writable) so it can wrap the NumPy buffer with zero copy. Inputs
# that don't meet those requirements are rejected with a descriptive error.
# Pass allow_copy=True to let the binding make a contiguous, writable copy
# automatically when the input is not directly shareable.

base = np.arange(12, dtype=np.float32)

# A non-contiguous slice would normally be rejected.
sliced = base[::2]
try:
    viskores.cont.array_from_numpy(sliced)
except RuntimeError as error:
    print(f"non-contiguous default: rejected ({str(error).splitlines()[0]})")

# allow_copy=True accepts the slice by copying it.
unknown = viskores.cont.array_from_numpy(sliced, allow_copy=True)
print(f"non-contiguous + allow_copy: {unknown}")
np.testing.assert_array_equal(unknown.asnumpy(), sliced)

# A directly-shareable input still takes the zero-copy path even with
# allow_copy=True. Only inputs that need a copy are copied.
unknown = viskores.cont.array_from_numpy(base, allow_copy=True)
print(f"contiguous + allow_copy: shares storage = "
      f"{np.shares_memory(unknown.asnumpy(), base)}")
