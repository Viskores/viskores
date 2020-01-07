# Shorter fancy array handle classnames

Many of the fancy `ArrayHandle`s use the generic builders like
`ArrayHandleTransform` and `ArrayHandleImplicit` for their implementation.
Such is fine, but because they use functors and other such generic items to
template their `Storage`, you can end up with very verbose classnames. This
is an issue for humans trying to discern classnames. It can also be an
issue for compilers that end up with very long resolved classnames that
might get truncated if they extend past what was expected.

The fix was for these classes to declare their own `Storage` tag and then
implement their `Storage` and `ArrayTransport` classes as trivial
subclasses of the generic `ArrayHandleImplicit` or `ArrayHandleTransport`.

As an added bonus, a lot of this shortening also means that storage that
relies on other array handles now are just typed by the storage of the
decorated type, not the array itself. This should make the types a little
more robust.

Here is a list of classes that were updated.

#### `ArrayHandleCast<TargetT, vtkm::cont::ArrayHandle<SourceT, SourceStorage>>`

Old storage: 
``` cpp
vtkm::cont::internal::StorageTagTransform<
  vtkm::cont::ArrayHandle<SourceT, SourceStorage>,
  vtkm::cont::internal::Cast<TargetT, SourceT>,
  vtkm::cont::internal::Cast<SourceT, TargetT>>
```

New Storage:
``` cpp
vtkm::cont::StorageTagCast<SourceT, SourceStorage>
```

(Developer's note: Implementing this change to `ArrayHandleCast` was a much bigger PITA than expected.)

#### `ArrayHandleCartesianProduct<AH1, AH2, AH3>`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagCartesianProduct<
  vtkm::cont::ArrayHandle<ValueType, StorageTag1,
  vtkm::cont::ArrayHandle<ValueType, StorageTag2,
  vtkm::cont::ArrayHandle<ValueType, StorageTag3>>
```

New storage:
``` cpp
vtkm::cont::StorageTagCartesianProduct<StorageTag1, StorageTag2, StorageTag3>
```

#### `ArrayHandleCompositeVector<AH1, AH2, ...>`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagCompositeVector<
  tao::tuple<
    vtkm::cont::ArrayHandle<ValueType, StorageType1>, 
	vtkm::cont::ArrayHandle<ValueType, StorageType2>,
	...
  >
>
```

New storage:
``` cpp
vtkm::cont::StorageTagCompositeVec<StorageType1, StorageType2>
```

#### `ArrayHandleConcatinate`

First an example with two simple types.

Old storage:
``` cpp
vtkm::cont::StorageTagConcatenate<
  vtkm::cont::ArrayHandle<ValueType, StorageTag1>,
  vtkm::cont::ArrayHandle<ValueType, StorageTag2>>
```

New storage:
``` cpp
vtkm::cont::StorageTagConcatenate<StorageTag1, StorageTag2>
```

Now a more specific example taken from the unit test of a concatination of a concatination.

Old storage:
``` cpp
vtkm::cont::StorageTagConcatenate<
  vtkm::cont::ArrayHandleConcatenate<
    vtkm::cont::ArrayHandle<ValueType, StorageTag1>,
	vtkm::cont::ArrayHandle<ValueType, StorageTag2>>,
  vtkm::cont::ArrayHandle<ValueType, StorageTag3>>
```

New storage:
``` cpp
vtkm::cont::StorageTagConcatenate<
  vtkm::cont::StorageTagConcatenate<StorageTag1, StorageTag2>, StorageTag3>
```

#### `ArrayHandleConstant`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<
  vtkm::cont::detail::ArrayPortalImplicit<
    vtkm::cont::detail::ConstantFunctor<ValueType>>>
```

New storage:
``` cpp
vtkm::cont::StorageTagConstant
```

#### `ArrayHandleCounting`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<vtkm::cont::internal::ArrayPortalCounting<ValueType>>
```

New storage:
``` cpp
vtkm::cont::StorageTagCounting
```

#### `ArrayHandleGroupVec`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagGroupVec<
  vtkm::cont::ArrayHandle<ValueType, StorageTag>, N>
```

New storage:
``` cpp
vtkm::cont::StorageTagGroupVec<StorageTag, N>
```

#### `ArrayHandleGroupVecVariable`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagGroupVecVariable<
  vtkm::cont::ArrayHandle<ValueType, StorageTag1>, 
  vtkm::cont::ArrayHandle<vtkm::Id, StorageTag2>>
```

New storage:
``` cpp
vtkm::cont::StorageTagGroupVecVariable<StorageTag1, StorageTag2>
```

#### `ArrayHandleIndex`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<
  vtkm::cont::detail::ArrayPortalImplicit<vtkm::cont::detail::IndexFunctor>>
```

New storage:
``` cpp
vtkm::cont::StorageTagIndex
```

#### `ArrayHandlePermutation`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagPermutation<
  vtkm::cont::ArrayHandle<vtkm::Id, StorageTag1>,
  vtkm::cont::ArrayHandle<ValueType, StorageTag2>>
```

New storage:
``` cpp
vtkm::cont::StorageTagPermutation<StorageTag1, StorageTag2>
```

#### `ArrayHandleReverse`

Old storage:
``` cpp
vtkm::cont::StorageTagReverse<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTag>>
```

New storage:
``` cpp
vtkm::cont::StorageTagReverse<StorageTag>
```

#### `ArrayHandleUniformPointCoordinates`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>
```

New Storage:
``` cpp
vtkm::cont::StorageTagUniformPoints
```

#### `ArrayHandleView`

Old storage:
``` cpp
vtkm::cont::StorageTagView<vtkm::cont::ArrayHandle<ValueType, StorageTag>>
```

New storage:
``` cpp
'vtkm::cont::StorageTagView<StorageTag>
```


#### `ArrayPortalZip`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagZip<
  vtkm::cont::ArrayHandle<ValueType1, StorageTag1>,
  vtkm::cont::ArrayHandle<ValueType2, StorageTag2>>
```

New storage:
``` cpp
vtkm::cont::StorageTagZip<StorageTag1, StorageTag2>
```
