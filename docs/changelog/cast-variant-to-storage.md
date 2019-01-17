# Allow VariantArrayHandle CastAndCall to cast to concrete types

Previously, the `VariantArrayHandle::CastAndCall` (and indirect calls through
`vtkm::cont::CastAndCall`) attempted to cast to only
`vtkm::cont::ArrayHandleVirtual` with different value types. That worked, but
it meant that whatever was called had to operate through virtual functions.

Under most circumstances, it is worthwhile to also check for some common
storage types that, when encountered, can be accessed much faster. This
change provides the casting to concrete storage types and now uses
`vtkm::cont::ArrayHandleVirtual` as a fallback when no concrete storage
type is found.

By default, `CastAndCall` checks all the storage types in
`VTKM_DEFAULT_STORAGE_LIST_TAG`, which typically contains only the basic
storage. The `ArrayHandleVirtual::CastAndCall` method also allows you to
override this behavior by specifying a different type list in the first
argument. If the first argument is a list type, `CastAndCall` assumes that
all the types in the list are storage tags. If you pass in
`vtkm::ListTagEmpty`, then `CastAndCall` will always cast to an
`ArrayHandleVirtual` (the previous behavior). Alternately, you can pass in
storage tags that might be likely under the current usage.

As an example, consider the following simple code.

``` cpp
vtkm::cont::VariantArrayHandle array;

// stuff happens

array.CastAndCall(myFunctor);
```

Previously, `myFunctor` would be called with
`vtkm::cont::ArrayHandleVirtual<T>` with different type `T`s. After this
change, `myFunctor` will be called with that and with
`vtkm::cont::ArrayHandle<T>` of the same type `T`s.

If you want to only call `myFunctor` with
`vtkm::cont::ArrayHandleVirtual<T>`, then replace the previous line with

``` cpp
array.CastAndCall(vtkm::ListTagEmpty(), myFunctor);
```

Let's say that additionally using `vtkm::cont::ArrayHandleIndex` was also
common. If you want to also specialize for that array, you can do so with
the following line.

``` cpp
array.CastAndCall(vtkm::ListTagBase<vtkm::cont::StorageBasic, 
                                    vtkm::cont::ArrayHandleIndex::StorageTag>,
                  myFunctor);
```

Note that `myFunctor` will be called with
`vtkm::cont::ArrayHandle<T,vtkm::cont::ArrayHandleIndex::StorageTag>`, not
`vtkm::cont::ArrayHandleIndex`.
