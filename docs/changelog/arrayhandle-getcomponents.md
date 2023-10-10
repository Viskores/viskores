# Add `GetNumberOfComponentsFlat` method to `ArrayHandle`

Getting the number of components (or the number of flattened components)
from an `ArrayHandle` is usually trivial. However, if the `ArrayHandle` is
special in that the number of components is specified at runtime, then it
becomes much more difficult to determine.

Getting the number of components is most important when extracting
component arrays (or reconstructions using component arrays) with
`UnknownArrayHandle`. Previously, `UnknownArrayHandle` used a hack to get
the number of components, which mostly worked but broke down when wrapping
a runtime array inside another array such as `ArrayHandleView`.

To prevent this issue, the ability to get the number of components has been
added to `ArrayHandle` proper. All `Storage` objects for `ArrayHandle`s now
need a method named `GetNumberOfComponentsFlat`. The implementation of this
method is usually trivial. The `ArrayHandle` template now also provides a
`GetNumberOfComponentsFlat` method that gets this information from the
`Storage`. This provides an easy access point for the `UnknownArrayHandle`
to pull this information.
