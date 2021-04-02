# Added VecFlat class

`vtkm::VecFlat` is a wrapper around a `Vec`-like class that may be a nested
series of vectors. For example, if you run a gradient operation on a vector
field, you are probably going to get a `Vec` of `Vec`s that looks something
like `vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 3>`. That is fine, but what if
you want to treat the result simply as a `Vec` of size 9?

The `VecFlat` wrapper class allows you to do this. Simply place the nested
`Vec` as an argument to `VecFlat` and it will behave as a flat `Vec` class.
(In fact, `VecFlat` is a subclass of `Vec`.) The `VecFlat` class can be
copied to and from the nested `Vec` it is wrapping.

There is a `vtkm::make_VecFlat` convenience function that takes an object
and returns a `vtkm::VecFlat` wrapped around it.

`VecFlat` works with any `Vec`-like object as well as scalar values.
However, any type used with `VecFlat` must have `VecTraits` defined and the
number of components must be static (i.e. known at compile time).
