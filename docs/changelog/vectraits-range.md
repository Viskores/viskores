# Add implementation of `VecTraits` for `Range` and `Bounds`

Added specializations of `vtkm::VecTraits` for the simple structures of
`vtkm::Range` and `vtkm::Bounds`. This expands the support for using these
structures in things like `ArrayHandle` and `UnknownArrayHandle`.

