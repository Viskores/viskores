## Fix compile issues with CUDA

CUDA 13.3 updated the Thrust library to version 3.3. This version changed the
use of an internal `is_commutative` traits class that Viskores specialized to
engage optimized paths of several algorithms. Viskore's use of this no longer
existing internal class caused compile errors.

Thrust moved to using the more general `cuda::is_commutative_v`, and Viskores
has followed by overloading this particular trait instead. As this class is no
longer marked as internal, hopefully it will be more stable across CUDA/Thrust
versions.

CUDA also changed how it handles visibility. Previous to CUDA 11, the nvcc
compiler did not support the `__attribute__((visibility("default")))` symbol
modifier. CUDA now does support this modifier, and with the most recent versions
it is important to add the default visibility to prevent symbols from being
hidden. Viskores now checks against the CUDA version to ensure that the proper
attribute (or lack thereof) is used for `VISKORES_ALWAYS_EXPORT`.
