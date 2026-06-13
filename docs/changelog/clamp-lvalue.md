## Fix Clamp handling of lvalue inputs

`viskores::Clamp` now returns a value type when called with lvalue inputs. This
prevents scalar clamps from returning references to temporary values, which could
produce unstable results on some platforms.

The fix also restores the return value from component-wise vector clamping.
Filters and worklets that clamp normalized floating-point values, such as
converting black RGB values to Lab colors, now produce stable results.
