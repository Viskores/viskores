# Fast paths for `ArrayRangeCompute` fixed

The precompiled `ArrayRangeCompute` function was not following proper fast
paths for special arrays. For example, when computing the range of an
`ArrayHandleUniformPointCoordinates`, the ranges should be taken from the
origin and spacing of the special array. However, the precompiled version
was calling the generic range computation, which was doing an unnecessary
reduction over the entire array. These fast paths have been fixed.

These mistakes in the code were caused by quirks in how templated method
overloading works. To prevent this mistake from happening again in the
precompiled `ArrayRangeCompute` function and elsewhere, all templated forms
of `ArrayRangeCompute` have been deprecated. Most will call
`ArrayRangeCompute` with no issues. For those that need the templated
version, `ArrayRangeComputeTemplate` replaces the old templated
`ArrayRangeCompute`. There is exactly one templated declaration of
`ArrayRangeComputeTemplate` that uses a class, `ArrayRangeComputeImpl`,
with partial specialization to ensure the correct form is used.
