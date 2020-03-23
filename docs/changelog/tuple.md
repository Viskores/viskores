# Add a vtkm::Tuple class

This change added a `vtkm::Tuple` class that is very similar in nature to
`std::tuple`. This should replace our use of tao tuple.

The motivation for this change was some recent attempts at removing objects
like `Invocation` and `FunctionInterface`. I expected these changes to
speed up the build, but in fact they ended up slowing down the build. I
believe the problem was that these required packing variable parameters
into a tuple. I was using the tao `tuple` class, but it seemed to slow down
the compile. (That is, compiling tao's `tuple` seemed much slower than
compiling the equivalent `FunctionInterface` class.)

The implementation of `vtkm::Tuple` is using `pyexpander` to build lots of
simple template cases for the object (with a backup implementation for even
longer argument lists). I believe the compiler is better and parsing
through thousands of lines of simple templates than to employ clever MPL to
build general templates.

## Usage

The `vtkm::Tuple` class is defined in the `vtkm::Tuple.h` header file. A
`Tuple` is designed to behave much like a `std::tuple` with some minor
syntax differences to fit VTK-m coding standards.

A tuple is declared with a list of template argument types.

``` cpp
vtkm::Tuple<vtkm::Id, vtkm::Vec3f, vtkm::cont::ArrayHandle<vtkm::Float32>> myTuple;
```

If given no arguments, a `vtkm::Tuple` will default-construct its contained
objects. A `vtkm::Tuple` can also be constructed with the initial values of
all contained objects.

``` cpp
vtkm::Tuple<vtkm::Id, vtkm::Vec3f, vtkm::cont::ArrayHandle<vtkm::Float32>> 
  myTuple(0, vtkm::Vec3f(0, 1, 2), array);
```

For convenience there is a `vtkm::MakeTuple` function that takes arguments
and packs them into a `Tuple` of the appropriate type. (There is also a
`vtkm::make_tuple` alias to the function to match the `std` version.)

``` cpp
auto myTuple = vtkm::MakeTuple(0, vtkm::Vec3f(0, 1, 2), array);
```

Data is retrieved from a `Tuple` by using the `vtkm::Get` method. The `Get`
method is templated on the index to get the value from. The index is of
type `vtkm::IdComponent`. (There is also a `vtkm::get` that uses a
`std::size_t` as the index type as an alias to the function to match the
`std` version.)

``` cpp
vtkm::Id a = vtkm::Get<0>(myTuple);
vtkm::Vec3f b = vtkm::Get<1>(myTuple);
vtkm::cont::ArrayHandle<vtkm::Float32> c = vtkm::Get<2>(myTuple);
```

Likewise `vtkm::TupleSize` and `vtkm::TupleElement` (and their aliases
`vtkm::Tuple_size`, `vtkm::tuple_element`, and `vtkm::tuple_element_t`) are
provided.

## Extended Functionality

The `vtkm::Tuple` class contains some functionality beyond that of
`std::tuple` to cover some common use cases in VTK-m that are tricky to
implement. In particular, these methods allow you to use a `Tuple` as you
would commonly use parameter packs. This allows you to stash parameter
packs in a `Tuple` and then get them back out again.

### For Each

`vtkm::Tuple::ForEach()` is a method that takes a function or functor and
calls it for each of the items in the tuple. Nothing is returned from
`ForEach` and any return value from the function is ignored.

`ForEach` can be used to check the validity of each item.

``` cpp
void CheckPositive(vtkm::Float64 x)
{
  if (x < 0)
  {
    throw vtkm::cont::ErrorBadValue("Values need to be positive.");
  }
}

// ...

  vtkm::Tuple<vtkm::Float64, vtkm::Float64, vtkm::Float64> tuple(
    CreateValue1(), CreateValue2(), CreateValue3());

  // Will throw an error if any of the values are negative.
  tuple.ForEach(CheckPositive);
```

`ForEach` can also be used to aggregate values.

``` cpp
struct SumFunctor
{
  vtkm::Float64 Sum = 0;
  
  template <typename T>
  void operator()(const T& x)
  {
    this->Sum = this->Sum + static_cast<vtkm::Float64>(x);
  }
};

// ...

  vtkm::Tuple<vtkm::Float32, vtkm::Float64, vtkm::Id> tuple(
    CreateValue1(), CreateValue2(), CreateValue3());

  SumFunctor sum;
  tuple.ForEach(sum);
  vtkm::Float64 average = sum.Sum / 3;
```

### Transform

`vtkm::Tuple::Transform` is a method that builds a new `Tuple` by calling a
function or functor on each of the items. The return value is placed in the
corresponding part of the resulting `Tuple`, and the type is automatically
created from the return type of the function.

``` cpp
struct GetReadPortalFunctor
{
  template <typename Array>
  typename Array::ReadPortal operator()(const Array& array) const
  {
    VTKM_IS_ARRAY_HANDLE(Array);
	return array.ReadPortal();
  }
};

// ...

  auto arrayTuple = vtkm::MakeTuple(array1, array2, array3);
  
  auto portalTuple = arrayTuple.Transform(GetReadPortalFunctor{});
```

### Apply

`vtkm::Tuple::Apply` is a method that calls a function or functor using the
objects in the `Tuple` as the arguments. If the function returns a value,
that value is returned from `Apply`.

``` cpp
struct AddArraysFunctor
{
  template <typename Array1, typename Array2, typename Array3>
  vtkm::Id operator()(Array1 inArray1, Array2 inArray2, Array3 outArray) const
  {
    VTKM_IS_ARRAY_HANDLE(Array1);
    VTKM_IS_ARRAY_HANDLE(Array2);
    VTKM_IS_ARRAY_HANDLE(Array3);

    vtkm::Id length = inArray1.GetNumberOfValues();
	VTKM_ASSERT(inArray2.GetNumberOfValues() == length);
	outArray.Allocate(length);
	
	auto inPortal1 = inArray1.ReadPortal();
	auto inPortal2 = inArray2.ReadPortal();
	auto outPortal = outArray.WritePortal();
	for (vtkm::Id index = 0; index < length; ++index)
	{
	  outPortal.Set(index, inPortal1.Get(index) + inPortal2.Get(index));
	}
	
	return length;
  }
};

// ...

  auto arrayTuple = vtkm::MakeTuple(array1, array2, array3);

  vtkm::Id arrayLength = arrayTuple.Apply(AddArraysFunctor{});
```

If additional arguments are given to `Apply`, they are also passed to the
function (before the objects in the `Tuple`). This is helpful for passing
state to the function. (This feature is not available in either `ForEach`
or `Transform` for technical implementation reasons.)

``` cpp
struct ScanArrayLengthFunctor
{
  template <std::size_t N, typename Array, typename... Remaining>
  std::array<vtkm::Id, N + 1 + sizeof...(Remaining)>
  operator()(const std::array<vtkm::Id, N>& partialResult,
             const Array& nextArray,
			 const Remaining&... remainingArrays) const
  {
    std::array<vtkm::Id, N + 1> nextResult;
	std::copy(partialResult.begin(), partialResult.end(), nextResult.begin());
    nextResult[N] = nextResult[N - 1] + nextArray.GetNumberOfValues();
	return (*this)(nextResult, remainingArray);
  }
  
  template <std::size_t N>
  std::array<vtkm::Id, N> operator()(const std::array<vtkm::Id, N>& result) const
  {
    return result;
  }
};

// ...

  auto arrayTuple = vtkm::MakeTuple(array1, array2, array3);
  
  std::array<vtkm::Id, 4> = 
    arrayTuple.Apply(ScanArrayLengthFunctor{}, std::array<vtkm::Id, 1>{ 0 });
```
