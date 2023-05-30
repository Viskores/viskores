# Fix an issue with copying array from a disabled device

The internal array copy has an optimization to use the device the array
exists on to do the copy. However, if that device is disabled the copy
would fail. This problem has been fixed.
