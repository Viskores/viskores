# Added support for getting vec sizes of unknown arrays when runtime selected

The `GetNumberOfComponents` and `GetNumberOfComponentsFlat` methods in
`UnknownArrayHandle` have been updated to correctly report the number of
components in special `ArrayHandle`s where the `Vec` sizes of the values
are not selected until runtime.

Previously, these methods always reported 0 because the value type could
not report the size of the `Vec`. The lookup has been modified to query the
`ArrayHandle`'s `Storage` for the number of components where supported.
Note that this only works on `Storage` that provides a method to get the
runtime `Vec` size. If that is not provided, as will be the case if the
number of components can vary from one value to the next, it will still
report 0.

This feature is implemented by looking for a method named
`GetNumberOfComponents` is the `Storage` class for the `ArrayHandle`. If
this method is found, it is used to query the size at runtime.
