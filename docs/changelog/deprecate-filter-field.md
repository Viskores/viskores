# Deprecated `vtkm::filter::FilterField`

The original design of the filter base class required several specialized
base classes to control what information was pulled from the input
`DataSet` and provided to the derived class. Since the filter base class was
redesigned, the derived classes all get a `DataSet` and pull their own
information from it. Thus, most specialized filter base classes became
unnecessary and removed.

The one substantial exception was the `FilterField`. This filter base class
managed input and output arrays. This was kept separate from the base
`Filter` because not all filters need the ability to select this
information.

That said, this separation has not been particularly helpful. There are
several other features of `Filter` that does not apply to all subclasses.
Furthermore, there are several derived filters that are using `FilterField`
merely to pick a single part, like selecting a coordinate system, and
ignoring the rest of the abilities.

Thus, it makes more sense to deprecate `FilterField` and have these classes
inherit directly from `Filter`.
