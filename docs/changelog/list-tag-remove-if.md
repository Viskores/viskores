# Add `ListTagRemoveIf`

It is sometimes useful to remove types from `ListTag`s. This is especially
the case when combining lists of types together where some of the type
combinations may be invalid and should be removed. To handle this
situation, a new `ListTag` type is added: `ListTagRemoveIf`.

`ListTagRemoveIf` is a template structure that takes two arguments. The
first argument is another `ListTag` type to operate on. The second argument
is a template that acts as a predicate. The predicate takes a type and
declares a Boolean `value` that should be `true` if the type should be
removed and `false` if the type should remain.

Here is an example of using `ListTagRemoveIf` to get basic types that hold
only integral values.

``` cpp
template <typename T>
using IsRealValue =
  std::is_same<
    typename vtkm::TypeTraits<typename vtkm::VecTraits<T>::BaseComponentType>::NumericTag,
    vtkm::TypeTraitsRealTag>;

using MixedTypes =
  vtkm::ListTagBase<vtkm::Id, vtkm::FloatDefault, vtkm::Id3, vtkm::Vec3f>;

using IntegralTypes = vtkm::ListTagRemoveIf<MixedTypes, IsRealValue>;
// IntegralTypes now equivalent to vtkm::ListTagBase<vtkm::Id, vtkm::Id3>
```
