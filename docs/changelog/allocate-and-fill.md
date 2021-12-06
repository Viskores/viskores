# ArrayHandle::Fill

`ArrayHandle` has a new method named `Fill`. As the name would suggest, the
`Fill` method initializes all elements in the array to a specified value.
In addition to being more convenient than calling `Algorithm::Fill` or
`ArrayCopy` with a constant array, the `ArrayHandle::Fill` can be used
without using a device adapter.

Calling `Fill` directly requires the `ArrayHandle` to first be allocated to
the appropriate size. The `ArrayHandle` now also has a new member named
`AllocateAndFill`. As the name would suggest, this method resizes the array
and then fills it with the specified value. Another feature this method has
over calling `Allocate` and `Fill` separately is if you call
`AllocateAndFill` with `vtkm::CopyFlag::On`, it will fill only the extended
portion of the array.

