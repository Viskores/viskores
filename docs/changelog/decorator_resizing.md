# ArrayHandleDecorator Allocate and Shrink Support

`ArrayHandleDecorator` can now be resized when given an appropriate 
decorator implementation.

Since the mapping between the size of an `ArrayHandleDecorator` and its source
`ArrayHandle`s is not well defined, resize operations (such as `Shrink` and 
`Allocate`) are not defined by default, and will throw an exception if called.

However, by implementing the methods `AllocateSourceArrays` and/or
`ShrinkSourceArrays` on the implementation class, resizing the decorator is 
allowed. These methods are passed in a new size along with each of the 
`ArrayHandleDecorator`'s source arrays, allowing developers to control how
the resize operation should affect the source arrays.

For example, the following decorator implementation can be used to create a
resizable `ArrayHandleDecorator` that is implemented using two arrays, which
are combined to produce values via the expression:

```
[decorator value i] = [source1 value i] * 10 + [source2 value i]
```

Implementation:

```c++
  template <typename ValueType>
  struct DecompositionDecorImpl
  {
    template <typename Portal1T, typename Portal2T>
    struct Functor
    {
      Portal1T Portal1;
      Portal2T Portal2;

      VTKM_EXEC_CONT
      ValueType operator()(vtkm::Id idx) const
      {
        return static_cast<ValueType>(this->Portal1.Get(idx) * 10 + this->Portal2.Get(idx));
      }
    };

    template <typename Portal1T, typename Portal2T>
    struct InverseFunctor
    {
      Portal1T Portal1;
      Portal2T Portal2;

      VTKM_EXEC_CONT
      void operator()(vtkm::Id idx, const ValueType& val) const
      {
        this->Portal1.Set(idx, static_cast<ValueType>(std::floor(val / 10)));
        this->Portal2.Set(idx, static_cast<ValueType>(std::fmod(val, 10)));
      }
    };

    template <typename Portal1T, typename Portal2T>
    VTKM_CONT Functor<typename std::decay<Portal1T>::type, typename std::decay<Portal2T>::type>
    CreateFunctor(Portal1T&& p1, Portal2T&& p2) const
    {
      return { std::forward<Portal1T>(p1), std::forward<Portal2T>(p2) };
    }

    template <typename Portal1T, typename Portal2T>
    VTKM_CONT InverseFunctor<typename std::decay<Portal1T>::type, typename std::decay<Portal2T>::type>
    CreateInverseFunctor(Portal1T&& p1, Portal2T&& p2) const
    {
      return { std::forward<Portal1T>(p1), std::forward<Portal2T>(p2) };
    }

    // Resize methods:
    template <typename Array1T, typename Array2T>
    VTKM_CONT
    void AllocateSourceArrays(vtkm::Id numVals, Array1T&& array1, Array2T&& array2) const
    {
      array1.Allocate(numVals);
      array2.Allocate(numVals);
    }

    template <typename Array1T, typename Array2T>
    VTKM_CONT
    void ShrinkSourceArrays(vtkm::Id numVals, Array1T&& array1, Array2T&& array2) const
    {
      array1.Shrink(numVals);
      array2.Shrink(numVals);
    }
  };

  // Usage:
  vtkm::cont::ArrayHandle<ValueType> a1;
  vtkm::cont::ArrayHandle<ValueType> a2;
  auto decor = vtkm::cont::make_ArrayHandleDecorator(0, DecompositionDecorImpl<ValueType>{}, a1, a2);
  
  decor.Allocate(5);
  {
    auto decorPortal = decor.GetPortalControl();
    decorPortal.Set(0, 13);
    decorPortal.Set(1, 8);
    decorPortal.Set(2, 43);
    decorPortal.Set(3, 92);
    decorPortal.Set(4, 117);
  }

  // a1:    {   1,   0,   4,   9,   11 }
  // a2:    {   3,   8,   3,   2,    7 }
  // decor: {  13,   8,  43,  92,  117 }
 
  decor.Shrink(3);

  // a1:    {   1,   0,   4 }
  // a2:    {   3,   8,   3 }
  // decor: {  13,   8,  43 }

```
