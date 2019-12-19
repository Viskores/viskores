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

#### `ArrayHandleCartesianProduct<AH1, AH2, AH3>

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

#### `ArrayHandleUniformPointCoordinates

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>
```

New Storage:
``` cpp
vtkm::cont::StorageTagUniformPoints
```
