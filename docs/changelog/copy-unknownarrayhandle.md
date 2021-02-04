# Precompiled `ArrayCopy` for `UnknownArrayHandle`

Previously, in order to copy an `UnknownArrayHandle`, you had to specify
some subset of types and then specially compile a copy for each potential
type. With the new ability to extract a component from an
`UnknownArrayHandle`, it is now feasible to precompile copying an
`UnknownArrayHandle` to another array. This greatly reduces the overhead of
using `ArrayCopy` to copy `UnknownArrayHandle`s while simultaneously
increasing the likelihood that the copy will be successful.
