# Improved = operators in VecFromPortal

Previously, `VecFromPortal` could only be set to a standard `Vec`.
However, because this is a `Vec`-like object with a runtime-size, it is
hard to do general arithmetic on it. It is easier to do in place so
there is some place to put the result. To make it easier to operate on
this as the result of other `Vec`-likes, extend the operators like `+=`,
`*=`, etc to support this.
