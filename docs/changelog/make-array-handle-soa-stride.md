## Fixed make_ArrayHandleSOAStride

The `make_ArrayHandleSOAStride` helper function had the wrong calling
specification and therefore could never be compiled correctly. The function is
fixed and now tested.

There has also been some documentation added to make this function easier to
use.
