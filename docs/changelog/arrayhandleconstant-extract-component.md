# Added support for component extraction from ArrayHandleConstant better

Previously, `ArrayHandleConstant` did not really support component
extraction. Instead, it let a fallback operation create a full array on
the CPU.

Component extraction is now quite efficient for `ArrayHandleConstant`. It
creates a basic `ArrayHandle` with one entry and sets a modulo on the
`ArrayHandleStride` to access that value for all indices.
