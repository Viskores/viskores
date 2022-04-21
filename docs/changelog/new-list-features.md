# New `vtkm::List` features

New features were added to those available in `vtkm/List.h`. These new
features provide new operations on lists.

## Reductions

The new `vtkm::ListReduce` allows a reduction on a list. This template
takes three arguments: a `vtkm::List`, an operation, and an initial value.
The operation is itself a template that has two type arguments.

`vtkm::ListReduce` applies the initial value and the first item of the list
to the operator. The result of that template is then iteratively applied to
the operator with the next item in the list and so on.

``` cpp
// Operation to use
template <typename T1, typename T2>
using Add = std::integral_constant<typename T1::type, T1::value + T2::value>;

using MyList = vtkm::List<std::integral_constant<int, 25>,
                          std::integral_constant<int, 60>,
                          std::integral_constant<int, 87>,
                          std::integral_constant<int, 62>>;

using MySum = vtkm::ListReduce<MyList, Add, std::integral_constant<int, 0>>;
// MySum becomes std::integral_constant<int, 234> (25+60+87+62 = 234)
```

## All and Any

Because they are very common, two reductions that are automatically
supported are `vtkm::ListAll` and `vtkm::ListAny`. These both take a
`vtkm::List` containing either `std::true_type` or `std::false_type` (or
some other "compatible" type that has a constant static `bool` named
`value`). `vtkm::ListAll` will become `std::false_type` if any of the
entries in the list are `std::false_type`. `vtkm::ListAny` becomes
`std::true_type` if any of the entires in the list are `std::true_type`.

``` cpp
using MyList = vtkm::List<std::integral_constant<int, 25>,
                          std::integral_constant<int, 60>,
                          std::integral_constant<int, 87>,
                          std::integral_constant<int, 62>>;

template <typename T>
using IsEven = std::integral_constant<bool, ((T % 2) == 0)>;

// Note that vtkm::ListTransform<MyList, IsEven> becomes
// vtkm::List<std::false_type, std::true_type, std::false_type, std::true_type>

using AllEven = vtkm::ListAll<vtkm::ListTransform<MyList, IsEven>>;
// AllEven becomes std::false_type

using AnyEven = vtkm::ListAny<vtkm::ListTransform<MyList, IsEven>>;
// AnyEven becomes std::true_type
```
