# `ArrayRangeCompute` works on any array type without compiling device code

Originally, `ArrayRangeCompute` required you to know specifically the
`ArrayHandle` type (value type and storage type) and to compile using any
device compiler. The method is changed to include only overloads that have
precompiled versions of `ArrayRangeCompute`.

Additionally, an `ArrayRangeCompute` overload that takes an
`UnknownArrayHandle` has been added. In addition to allowing you to compute
the range of arrays of unknown types, this implementation of
`ArrayRangeCompute` serves as a fallback for `ArrayHandle` types that are
not otherwise explicitly supported.

If you really want to make sure that you compute the range directly on an
`ArrayHandle` of a particular type, you can include
`ArrayRangeComputeTemplate.h`, which contains a templated overload of
`ArrayRangeCompute` that directly computes the range of an `ArrayHandle`.
Including this header requires compiling for device code.
