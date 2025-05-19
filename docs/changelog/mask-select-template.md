## Added a template constructor to MaskSelect

The `viskores::worklet::MaskSelect` constructor takes a mask array and
constructs a lookup array for the mask operation. To speed up compilation and
reduce executable sizes, the building on the mask array is pre-compiled for
expected array types. However, it might be that you would like to construct a
mask from a selection array with other types such as
`viskores::cont::ArrayHandleTransform`. To do this efficiently, you need to
compile a special version of the mask build function. To allow this, a
`MaskSelectTemplate` class is added that has a templated version of the
constructor.
