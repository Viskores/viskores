# Replaced `vtkm::ListTag` with `vtkm::List`

The original `vtkm::ListTag` was designed when we had to support compilers
that did not provide C++11's variadic templates. Thus, the design hides
type lists, which were complicated to support.

Now that we support C++11, variadic templates are trivial and we can easily
create templated type aliases with `using`. Thus, it is now simpler to deal
with a template that lists types directly.

Hence, `vtkm::ListTag` is deprecated and `vtkm::List` is now supported. The
main difference between the two is that whereas `vtkm::ListTag` allowed you
to create a list by subclassing another list, `vtkm::List` cannot be
subclassed. (Well, it can be subclassed, but the subclass ceases to be
considered a list.) Thus, where before you would declare a list like

``` cpp
struct MyList : vtkm::ListTagBase<Type1, Type2, Type3>
{
};
```

you now make an alias

``` cpp
using MyList = vtkm::List<Type1, Type2, Type3>;
```

If the compiler reports the `MyList` type in an error or warning, it
actually uses the fully qualified `vtkm::List<Type1, Type2, Type3>`.
Although this makes errors more verbose, it makes it easier to diagnose
problems because the types are explicitly listed.

The new `vtkm::List` comes with a list of utility templates to manipulate
lists that mostly mirrors those in `vtkm::ListTag`: `VTKM_IS_LIST`,
`ListApply`, `ListSize`, `ListAt`, `ListIndexOf`, `ListHas`, `ListAppend`,
`ListIntersect`, `ListTransform`, `ListRemoveIf`, and `ListCross`. All of
these utilities become `vtkm::List<>` types (where applicable), which makes
them more consistent than the old `vtkm::ListTag` versions.

Thus, if you have a declaration like

``` cpp
vtkm::ListAppend(vtkm::List<Type1a, Type2a>, vtkm::List<Type1b, Type2b>>
```

this gets changed automatically to

``` cpp
vtkm::List<Type1a, Type2a, Type1b, Type2b>
```

This is in contrast to the equivalent old version, which would create a new
type for `vtkm::ListTagAppend` in addition to the ultimate actual list it
constructs.
