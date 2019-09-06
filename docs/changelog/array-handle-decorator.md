# Add `ArrayHandleDecorator`.

`ArrayHandleDecorator` is given a `DecoratorImpl` class and a list of one or
more source `ArrayHandle`s. There are no restrictions on the size or type of
the source `ArrayHandle`s.


The decorator implementation class is described below:

```
struct ExampleDecoratorImplementation
{

  // Takes one portal for each source array handle (only two shown).
  // Returns a functor that defines:
  //
  // ValueType operator()(vtkm::Id id) const;
  //
  // which takes an index and returns a value which should be produced by
  // the source arrays somehow. This ValueType will be the ValueType of the
  // ArrayHandleDecorator.
  //
  // Both SomeFunctor::operator() and CreateFunctor must be const.
  //
  template <typename Portal1Type, typename Portal2Type>
  SomeFunctor CreateFunctor(Portal1Type portal1, Portal2Type portal2) const;

  // Takes one portal for each source array handle (only two shown).
  // Returns a functor that defines:
  //
  // void operator()(vtkm::Id id, ValueType val) const;
  //
  // which takes an index and a value, which should be used to modify one
  // or more of the source arrays.
  //
  // CreateInverseFunctor is optional; if not provided, the
  // ArrayHandleDecorator will be read-only. In addition, if all of the
  // source ArrayHandles are read-only, the inverse functor will not be used
  // and the ArrayHandleDecorator will be read only.
  //
  // Both SomeInverseFunctor::operator() and CreateInverseFunctor must be
  // const.
  //
  template <typename Portal1Type, typename Portal2Type>
  SomeInverseFunctor CreateInverseFunctor(Portal1Type portal1,
                                          Portal2Type portal2) const;

};
```

Some example implementation classes are provided below:

Reverse a ScanExtended:
```
// Decorator implementation that reverses the ScanExtended operation.
//
// The resulting ArrayHandleDecorator will take an array produced by the
// ScanExtended algorithm and return the original ScanExtended input.
//
// Some interesting things about this:
// - The ArrayHandleDecorator's ValueType will not be the same as the
//   ScanPortal's ValueType. The Decorator ValueType is determined by the
//   return type of Functor::operator().
// - The ScanPortal has more values than the ArrayHandleDecorator. The
//   number of values the ArrayHandleDecorator should hold is set during
//   construction and may differ from the arrays it holds.
template <typename ValueType>
struct ScanExtendedToNumIndicesDecorImpl
{
  template <typename ScanPortalType>
  struct Functor
  {
    ScanPortalType ScanPortal;

    VTKM_EXEC_CONT
    ValueType operator()(vtkm::Id idx) const
    {
      return static_cast<ValueType>(this->ScanPortal.Get(idx + 1) -
                                    this->ScanPortal.Get(idx));
    }
  };

  template <typename ScanPortalType>
  Functor<ScanPortalType> CreateFunctor(ScanPortalType portal) const
  {
    return {portal};
  }
};

auto numIndicesOrig = vtkm::cont::make_ArrayHandleCounting(ValueType{0},
                                                           ValueType{1},
                                                           ARRAY_SIZE);
vtkm::cont::ArrayHandle<vtkm::Id> scan;
vtkm::cont::Algorithm::ScanExtended(
      vtkm::cont::make_ArrayHandleCast<vtkm::Id>(numIndicesOrig),
      scan);
auto numIndicesDecor = vtkm::cont::make_ArrayHandleDecorator(
      ARRAY_SIZE,
      ScanExtendedToNumIndicesDecorImpl<ValueType>{},
      scan);
```

Combine two other `ArrayHandle`s using an arbitrary binary operation:
```
// Decorator implementation that demonstrates how to create functors that
// hold custom state. Here, the functors have a customizable Operation
// member.
//
// This implementation is used to create a read-only ArrayHandleDecorator
// that combines the values in two other ArrayHandles using an arbitrary
// binary operation (e.g. vtkm::Maximum, vtkm::Add, etc).
template <typename ValueType, typename OperationType>
struct BinaryOperationDecorImpl
{
  OperationType Operation;

  // The functor use to read values. Note that it holds extra state in
  // addition to the portals.
  template <typename Portal1Type, typename Portal2Type>
  struct Functor
  {
    Portal1Type Portal1;
    Portal2Type Portal2;
    OperationType Operation;

    VTKM_EXEC_CONT
    ValueType operator()(vtkm::Id idx) const
    {
      return this->Operation(static_cast<ValueType>(this->Portal1.Get(idx)),
                             static_cast<ValueType>(this->Portal2.Get(idx)));
    }
  };

  // A non-variadic example of a factory function to produce a functor. This
  // is where the extra state is passed into the functor.
  template <typename P1T, typename P2T>
  Functor<P1T, P2T> CreateFunctor(P1T p1, P2T p2) const
  {
    return {p1, p2, this->Operation};
  }
};

BinaryOperationDecorImpl<ValueType, vtkm::Maximum> factory{vtkm::Maximum{}};
auto decorArray = vtkm::cont::make_ArrayHandleDecorator(ARRAY_SIZE,
                                                        factory,
                                                        array1,
                                                        array2);
```

A factory that does a complex and invertible operation on three portals:

```
// Decorator implemenation that demonstrates how to write invertible functors
// that combine three array handles with complex access logic. The resulting
// ArrayHandleDecorator can be both read from and written to.
//
// Constructs functors that take three portals.
//
// The first portal's values are accessed in reverse order.
// The second portal's values are accessed in normal order.
// The third portal's values are accessed via ((idx + 3) % size).
//
// Functor will return the max of the first two added to the third.
//
// InverseFunctor will update the third portal such that the Functor would
// return the indicated value.
struct InvertibleDecorImpl
{

  // The functor used for reading data from the three portals.
  template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
  struct Functor
  {
    using ValueType = typename Portal1Type::ValueType;

    Portal1Type Portal1;
    Portal2Type Portal2;
    Portal3Type Portal3;

    VTKM_EXEC_CONT ValueType operator()(vtkm::Id idx) const
    {
      const auto idx1 = this->Portal1.GetNumberOfValues() - idx - 1;
      const auto idx2 = idx;
      const auto idx3 = (idx + 3) % this->Portal3.GetNumberOfValues();

      const auto v1 = this->Portal1.Get(idx1);
      const auto v2 = this->Portal2.Get(idx2);
      const auto v3 = this->Portal3.Get(idx3);

      return vtkm::Max(v1, v2) + v3;
    }
  };

  // The functor used for writing. Only Portal3 is written to, the other
  // portals may be read-only.
  template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
  struct InverseFunctor
  {
    using ValueType = typename Portal1Type::ValueType;

    Portal1Type Portal1;
    Portal2Type Portal2;
    Portal3Type Portal3;

    VTKM_EXEC_CONT void operator()(vtkm::Id idx, const ValueType &vIn) const
    {
      const auto v1 = this->Portal1.Get(this->Portal1.GetNumberOfValues() - idx - 1);
      const auto v2 = this->Portal2.Get(idx);
      const auto vNew = static_cast<ValueType>(vIn - vtkm::Max(v1, v2));
      this->Portal3.Set((idx + 3) % this->Portal3.GetNumberOfValues(), vNew);
    }
  };

  // Factory function that takes 3 portals as input and creates an instance
  // of Functor with them. Variadic template parameters are used here, but are
  // not necessary.
  template <typename... PortalTs>
  Functor<typename std::decay<PortalTs>::type...>
  CreateFunctor(PortalTs&&... portals) const
  {
    VTKM_STATIC_ASSERT(sizeof...(PortalTs) == 3);
    return {std::forward<PortalTs>(portals)...};
  }

  // Factory function that takes 3 portals as input and creates an instance
  // of InverseFunctor with them. Variadic template parameters are used here,
  // but are not necessary.
  template <typename... PortalTs>
  InverseFunctor<typename std::decay<PortalTs>::type...>
  CreateInverseFunctor(PortalTs&&... portals) const
  {
    VTKM_STATIC_ASSERT(sizeof...(PortalTs) == 3);
    return {std::forward<PortalTs>(portals)...};
  }
};

// Note that only ah3 must be writable for ahInv to be writable. ah1 and ah2
// may be read-only arrays.
auto ahInv = vtkm::cont::make_ArrayHandleDecorator(ARRAY_SIZE,
                                                   InvertibleDecorImpl{},
                                                   ah1,
                                                   ah2,
                                                   ah3);
```
