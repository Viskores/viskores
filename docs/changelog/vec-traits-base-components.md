# Recursive base component queries to VecTraits

This change adds a recursive `BaseComponentType` to `VecTraits` that
recursively finds the base (non-`Vec`) type of a `Vec`. This is useful when
dealing with potentially nested `Vec`s (e.g. `Vec<Vec<T, M>, N>`) and you
want to know the precision of the math being defined.

``` cpp
using NestedVec = vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 8>;

// ComponentType becomes vtkm::Vec<vtkm::Float32, 3>
using ComponentType = typename vtkm::VecTraits<NestedVec>::ComponentType;

// BaseComponentType becomes vtkm::Float32
using BaseComponentType = typename vtkm::VecTraits<NestedVec>::BaseComponentType;
```

Also added the ability to `VecTraits` to change the component type of a
vector. The template `RepalceComponentType` resolves to a `Vec` of the same
type with the component replaced with a new type. The template
`ReplaceBaseComponentType` traverses down a nested type and replaces the
base type.

``` cpp
using NestedVec = vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 8>;

// NewVec1 becomes vtkm::Vec<vtkm::Float64, 8>
using NewVec1 =
  typename vtkm::VecTraits<NestedVec>::template ReplaceComponentType<vtkm::Float64>;

// NewVec2 becomes vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 8>
using NewVec1 =
  typename vtkm::VecTraits<NestedVec>::template ReplaceBaseComponentType<vtkm::Float64>;
```

This functionality replaces the functionality in `vtkm::BaseComponent`. Unfortunately,
`vtkm::BaseComponent` did not have the ability to replace the base component and
there was no straightforward way to implement that outside of `VecTraits`.

