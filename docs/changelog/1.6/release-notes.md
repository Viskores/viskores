VTK-m 1.6 Release Notes
=======================

# Table of Contents

1. [Core](#Core)
  - Add Kokkos backend
  - Deprecate `DataSetFieldAdd`
  - Move VTK file readers and writers into vtkm_io
  - Remove VTKDataSetWriter::WriteDataSet just_points parameter
  - Added VecFlat class
  - Add a vtkm::Tuple class
  - DataSet now only allows unique field names
  - Result DataSet of coordinate transform has its CoordinateSystem changed
  - `vtkm::cont::internal::Buffer` now can have ownership transferred
  - Configurable default types
2. [ArrayHandle](#ArrayHandle)
  - Shorter fancy array handle classnames
  - `ReadPortal().Get(idx)`
  - Precompiled `ArrayCopy` for `UnknownArrayHandle`
  - Create `ArrayHandleOffsetsToNumComponents`
  - Recombine extracted component arrays from unknown arrays
  - UnknownArrayHandle and UncertainArrayHandle for runtime-determined types
  - Support `ArrayHandleSOA` as a "default" array
  - Removed old `ArrayHandle` transfer mechanism
  - Order asynchronous `ArrayHandle` access
  - Improvements to moving data into ArrayHandle
  - Deprecate ArrayHandleVirtualCoordinates
  - ArrayHandleDecorator Allocate and Shrink Support
  - Portals may advertise custom iterators
  - Redesign of ArrayHandle to access data using typeless buffers
  - `ArrayRangeCompute` works on any array type without compiling device code
  - Implemented ArrayHandleRandomUniformBits and ArrayHandleRandomUniformReal
  - Extract component arrays from unknown arrays
  - `ArrayHandleGroupVecVariable` holds now one more offset.
3. [Control Environment](#Control-Environment)
  - Algorithms for Control and Execution Environments
4. [Execution Environment](#Execution-Environment)
  - Scope ExecObjects with Tokens
  - Masks and Scatters Supported for 3D Scheduling
  - Virtual methods in execution environment deprecated
  - Deprecate Execute with policy
5. [Worklets and Filters](#Worklets-and-Filters)
  - Enable setting invalid value in probe filter
  - Avoid raising errors when operating on cells
  - Add atomic free functions
  - Flying Edges
  - Filters specify their own field types
6. [Build](#Build)
  - Disable asserts for CUDA architecture builds
  - Disable asserts for HIP architecture builds
  - Add VTKM_DEPRECATED macro
7. [Other](#Other)
  - Porting layer for future std features
  - Removed OpenGL Rendering Classes
  - Reorganization of `io` directory
  - Implemented PNG/PPM image Readers/Writers
  - Updated Benchmark Framework
  - Provide scripts to build Gitlab-ci workers locally
  - Replaced `vtkm::ListTag` with `vtkm::List`
  - Add `ListTagRemoveIf`
  - Write uniform and rectilinear grids to legacy VTK files
8. [References](#References)

# Core

## Add Kokkos backend

  Adds a new device backend `Kokkos` which uses the kokkos library for parallelism.
  User must provide the kokkos build and Vtk-m will use the default configured execution
  space.

## Deprecate `DataSetFieldAdd`

The class `vtkm::cont::DataSetFieldAdd` is now deprecated.
Its methods, `AddPointField` and `AddCellField` have been moved to member functions
of `vtkm::cont::DataSet`, which simplifies many calls.

For example, the following code

```cpp
vtkm::cont::DataSetFieldAdd fieldAdder;
fieldAdder.AddCellField(dataSet, "cellvar", values);
```

would now be

```cpp
dataSet.AddCellField("cellvar", values);
```

## Move VTK file readers and writers into vtkm_io

The legacy VTK file reader and writer were created back when VTK-m was a
header-only library. Things have changed and we now compile quite a bit of
code into libraries. At this point, there is no reason why the VTK file
reader/writer should be any different.

Thus, `VTKDataSetReader`, `VTKDataSetWriter`, and several supporting
classes are now compiled into the `vtkm_io` library. Also similarly updated
`BOVDataSetReader` for good measure.

As a side effect, code using VTK-m will need to link to `vtkm_io` if they
are using any readers or writers.


## Remove VTKDataSetWriter::WriteDataSet just_points parameter

In the method `VTKDataSetWriter::WriteDataSet`, `just_points` parameter has been
removed due to lack of usage. 

The purpose of `just_points` was to allow exporting only the points of a
DataSet without its cell data.


## Added VecFlat class

`vtkm::VecFlat` is a wrapper around a `Vec`-like class that may be a nested
series of vectors. For example, if you run a gradient operation on a vector
field, you are probably going to get a `Vec` of `Vec`s that looks something
like `vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 3>`. That is fine, but what if
you want to treat the result simply as a `Vec` of size 9?

The `VecFlat` wrapper class allows you to do this. Simply place the nested
`Vec` as an argument to `VecFlat` and it will behave as a flat `Vec` class.
(In fact, `VecFlat` is a subclass of `Vec`.) The `VecFlat` class can be
copied to and from the nested `Vec` it is wrapping.

There is a `vtkm::make_VecFlat` convenience function that takes an object
and returns a `vtkm::VecFlat` wrapped around it.

`VecFlat` works with any `Vec`-like object as well as scalar values.
However, any type used with `VecFlat` must have `VecTraits` defined and the
number of components must be static (i.e. known at compile time).


## Add a vtkm::[Tuple](Tuple) class

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

### Usage

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

### Extended Functionality

The `vtkm::Tuple` class contains some functionality beyond that of
`std::tuple` to cover some common use cases in VTK-m that are tricky to
implement. In particular, these methods allow you to use a `Tuple` as you
would commonly use parameter packs. This allows you to stash parameter
packs in a `Tuple` and then get them back out again.

#### For Each

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

#### Transform

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

#### Apply

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

## DataSet now only allows unique field names

When you add a `vtkm::cont::Field` to a `vtkm::cont::DataSet`, it now
requires every `Field` to have a unique name. When you attempt to add a
`Field` to a `DataSet` that already has a `Field` of the same name and
association, the old `Field` is removed and replaced with the new `Field`.

You are allowed, however, to have two `Field`s with the same name but
different associations. For example, you could have a point `Field` named
"normals" and also have a cell `Field` named "normals" in the same
`DataSet`.

This new behavior matches how VTK's data sets manage fields.

The old behavior allowed you to add multiple `Field`s with the same name,
but it would be unclear which one you would get if you asked for a `Field`
by name.


## Result DataSet of coordinate transform has its CoordinateSystem changed

When you run one of the coordinate transform filters,
`CylindricalCoordinateTransform` or `SphericalCoordinateTransform`, the
transform coordiantes are placed as the first `CoordinateSystem` in the
returned `DataSet`. This means that after running this filter, the data
will be moved to this new coordinate space.

Previously, the result of these filters was just placed in a named `Field`
of the output. This caused some confusion because the filter did not seem
to have any effect (unless you knew to modify the output data). Not using
the result as the coordinate system seems like a dubious use case (and not
hard to work around), so this is much better behavior.


## `vtkm::cont::internal::Buffer` now can have ownership transferred

Memory once transferred to `Buffer` always had to be managed by VTK-m. This is problematic
for applications that needed VTK-m to allocate memory, but have the memory ownership
be longer than VTK-m.

`Buffer::TakeHostBufferOwnership` allows for easy transfer ownership of memory out of VTK-m.
When taking ownership of an VTK-m buffer you are provided the following information:

- Memory: A `void*` pointer to the array
- Container: A `void*` pointer used to free the memory. This is necessary to support cases such as allocations transferred into VTK-m from a `std::vector`.
- Delete: The function to call to actually delete the transferred memory
- Reallocate: The function to call to re-allocate the transferred memory. This will throw an exception if users try
to reallocate a buffer that was 'view' only
- Size: The size in number of elements of the array 

 
To properly steal memory from VTK-m you do the following:
```cpp
  vtkm::cont::ArrayHandle<T> arrayHandle;

  ...

  auto stolen = arrayHandle.GetBuffers()->TakeHostBufferOwnership();
    
  ...

  stolen.Delete(stolen.Container);
```


## Configurable default types

Because VTK-m compiles efficient code for accelerator architectures, it
often has to compile for static types. This means that dynamic types often
have to be determined at runtime and converted to static types. This is the
reason for the `CastAndCall` architecture in VTK-m.

For this `CastAndCall` to work, there has to be a finite set of static
types to try at runtime. If you don't compile in the types you need, you
will get runtime errors. However, the more types you compile in, the longer
the compile time and executable size. Thus, getting the types right is
important.

The "right" types to use can change depending on the application using
VTK-m. For example, when VTK links in VTK-m, it needs to support lots of
types and can sacrifice the compile times to do so. However, if using VTK-m
in situ with a fortran simulation, space and time are critical and you
might only need to worry about double SoA arrays.

Thus, it is important to customize what types VTK-m uses based on the
application. This leads to the oxymoronic phrase of configuring the default
types used by VTK-m.

This is being implemented by providing VTK-m with a header file that
defines the default types. The header file provided to VTK-m should define
one or more of the following preprocessor macros:

  * `VTKM_DEFAULT_TYPE_LIST` - a `vtkm::List` of value types for fields that
     filters should directly operate on (where applicable).
  * `VTKM_DEFAULT_STORAGE_LIST` - a `vtkm::List` of storage tags for fields
     that filters should directly operate on.
  * `VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED` - a `vtkm::List` of
     `vtkm::cont::CellSet` types that filters should operate on as a
     strutured cell set.
  * `VTKM_DEFAULT_CELL_SET_LIST_UNSTRUCTURED` - a `vtkm::List` of
     `vtkm::cont::CellSet` types that filters should operate on as an
     unstrutured cell set.
  * `VTKM_DEFAULT_CELL_SET_LIST` - a `vtkm::List` of `vtkm::cont::CellSet`
     types that filters should operate on (where applicable). The default of
     `vtkm::ListAppend<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED, VTKM_DEFAULT_CELL_SET_LIST>`
	 is usually correct.

If any of these macros are not defined, a default version will be defined.
(This is the same default used if no header file is provided.)

This header file is provided to the build by setting the
`VTKm_DEFAULT_TYPES_HEADER` CMake variable. `VTKm_DEFAULT_TYPES_HEADER`
points to the file, which will be configured and copied to VTK-m's build
directory.

For convenience, header files can be added to the VTK_m source directory
(conventionally under vtkm/cont/internal). If this is the case, an advanced
CMake option should be added to select the provided header file.



# ArrayHandle

## Shorter fancy array handle classnames

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

### `ArrayHandleCast<TargetT, vtkm::cont::ArrayHandle<SourceT, SourceStorage>>`

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

### `ArrayHandleCartesianProduct<AH1, AH2, AH3>`

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

### `ArrayHandleCompositeVector<AH1, AH2, ...>`

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

### `ArrayHandleConcatinate`

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

### `ArrayHandleConstant`

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

### `ArrayHandleCounting`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<vtkm::cont::internal::ArrayPortalCounting<ValueType>>
```

New storage:
``` cpp
vtkm::cont::StorageTagCounting
```

### `ArrayHandleGroupVec`

Old storage:
``` cpp
vtkm::cont::internal::StorageTagGroupVec<
  vtkm::cont::ArrayHandle<ValueType, StorageTag>, N>
```

New storage:
``` cpp
vtkm::cont::StorageTagGroupVec<StorageTag, N>
```

### `ArrayHandleGroupVecVariable`

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

### `ArrayHandleIndex`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<
  vtkm::cont::detail::ArrayPortalImplicit<vtkm::cont::detail::IndexFunctor>>
```

New storage:
``` cpp
vtkm::cont::StorageTagIndex
```

### `ArrayHandlePermutation`

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

### `ArrayHandleReverse`

Old storage:
``` cpp
vtkm::cont::StorageTagReverse<vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTag>>
```

New storage:
``` cpp
vtkm::cont::StorageTagReverse<StorageTag>
```

### `ArrayHandleUniformPointCoordinates`

Old storage:
``` cpp
vtkm::cont::StorageTagImplicit<vtkm::internal::ArrayPortalUniformPointCoordinates>
```

New Storage:
``` cpp
vtkm::cont::StorageTagUniformPoints
```

### `ArrayHandleView`

Old storage:
``` cpp
vtkm::cont::StorageTagView<vtkm::cont::ArrayHandle<ValueType, StorageTag>>
```

New storage:
``` cpp
'vtkm::cont::StorageTagView<StorageTag>
```


### `ArrayPortalZip`

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


## `ReadPortal().Get(idx)`

Calling `ReadPortal()` in a tight loop is an antipattern.
A call to `ReadPortal()` causes the array to be copied back to the control environment,
and hence code like

```cpp
for (vtkm::Id i = 0; i < array.GetNumberOfValues(); ++i) {
    vtkm::FloatDefault x = array.ReadPortal().Get(i);
}
```

is a quadratic-scaling loop.

We have remove *almost* all internal uses of the `ReadPortal().Get` antipattern,
with the exception of 4 API calls into which the pattern is baked in:
`CellSetExplicit::GetCellShape`, `CellSetPermutation::GetNumberOfPointsInCell`, `CellSetPermutation::GetCellShape`, and `CellSetPermutation::GetCellPointIds`.
We expect these will need to be deprecated in the future.

## Precompiled `ArrayCopy` for `UnknownArrayHandle`

Previously, in order to copy an `UnknownArrayHandle`, you had to specify
some subset of types and then specially compile a copy for each potential
type. With the new ability to extract a component from an
`UnknownArrayHandle`, it is now feasible to precompile copying an
`UnknownArrayHandle` to another array. This greatly reduces the overhead of
using `ArrayCopy` to copy `UnknownArrayHandle`s while simultaneously
increasing the likelihood that the copy will be successful.

## Create `ArrayHandleOffsetsToNumComponents`

`ArrayHandleOffsetsToNumComponents` is a fancy array that takes an array of
offsets and converts it to an array of the number of components for each
packed entry.

It is common in VTK-m to pack small vectors of variable sizes into a single
contiguous array. For example, cells in an explicit cell set can each have
a different amount of vertices (triangles = 3, quads = 4, tetra = 4, hexa =
8, etc.). Generally, to access items in this list, you need an array of
components in each entry and the offset for each entry. However, if you
have just the array of offsets in sorted order, you can easily derive the
number of components for each entry by subtracting adjacent entries. This
works best if the offsets array has a size that is one more than the number
of packed vectors with the first entry set to 0 and the last entry set to
the total size of the packed array (the offset to the end).

When packing data of this nature, it is common to start with an array that
is the number of components. You can convert that to an offsets array using
the `vtkm::cont::ConvertNumComponentsToOffsets` function. This will create
an offsets array with one extra entry as previously described. You can then
throw out the original number of components array and use the offsets with
`ArrayHandleOffsetsToNumComponents` to represent both the offsets and num
components while storing only one array.

This replaces the use of `ArrayHandleDecorator` in `CellSetExplicit`.
The two implementation should do the same thing, but the new
`ArrayHandleOffsetsToNumComponents` should be less complex for
compilers.

## Recombine extracted component arrays from unknown arrays

Building on the recent capability to [extract component arrays from unknown
arrays](array-extract-component.md), there is now also the ability to
recombine these extracted arrays to a single `ArrayHandle`. It might seem
counterintuitive to break an `ArrayHandle` into component arrays and then
combine the component arrays back into a single `ArrayHandle`, but this is
a very handy way to run algorithms without knowing the exact `ArrayHandle`
type.

Recall that when extracting a component array from an `UnknownArrayHandle`
you only need to know the base component of the value type of the contained
`ArrayHandle`. That makes extracting a component array independent from
either the size of any `Vec` value type and any storage type.

The added `UnknownArrayHandle::ExtractArrayFromComponents` method allows
you to use the functionality to transform the unknown array handle to a
form of `ArrayHandle` that depends only on this base component type. This
method internally uses a new `ArrayHandleRecombineVec` class, but this
class is mostly intended for internal use by this class.

As an added convenience, `UnknownArrayHandle` now also provides the
`CastAndCallWithExtractedArray` method. This method works like other
`CastAndCall`s except that it uses the `ExtractArrayFromComponents` feature
to allow you to handle most `ArrayHandle` types with few template
instances.


## UnknownArrayHandle and UncertainArrayHandle for runtime-determined types

Two new classes have been added to VTK-m: `UnknownArrayHandle` and
`UncertainArrayHandle`. These classes serve the same purpose as the set of
`VariantArrayHandle` classes and will replace them.

Motivated mostly by the desire to move away from `ArrayHandleVirtual`, we
have multiple reasons to completely refactor the `VariantArrayHandle`
class. These include changing the implementation, some behavior, and even
the name.

### Motivation

We have several reasons that have accumulated to revisit the implementation
of `VariantArrayHandle`.

#### Move away from `ArrayHandleVirtual`

The current implementation of `VariantArrayHandle` internally stores the
array wrapped in an `ArrayHandleVirtual`. That makes sense since you might
as well consolidate the hierarchy of virtual objects into one.

Except `ArrayHandleVirtual` is being deprecated, so it no longer makes
sense to use that internally.

So we will transition the class back to managing the data as typeless on
its own. We will consider using function pointers rather than actual
virtual functions because compilers can be slow in creating lots of virtual
subclasses.

#### Reintroduce storage tag lists

The original implementation of `VariantArrayHandle` (which at the time was
called `DynamicArrayHandle`) actually had two type lists: one for the array
value type and one for the storage type. The storage type list was removed
soon after `ArrayHandleVirtual` was introduced because whatever the type of
array it could be access as `ArrayHandleVirtual`.

However, with `ArrayHandleVirtual` being deprecated, this feature is no
longer possible. We are in need again for the list of storage types to try.
Thus, we need to reintroduce this template argument to
`VariantArrayHandle`.

#### More clear name

The name of this class has always been unsatisfactory. The first name,
`DynamicArrayHandle`, makes it sound like the data is always changing. The
second name, `VariantArrayHandle`, makes it sound like an array that holds
a value type that can vary (like an `std::variant`).

We can use a more clear name that expresses better that it is holding an
`ArrayHandle` of an _unknown_ type.

#### Take advantage of default types for less templating

Once upon a time everything in VTK-m was templated header library. Things
have changed quite a bit since then. The most recent development is the
ability to select the "default types" with CMake configuration that allows
you to select a global set of types you care about during compilation. This
is so units like filters can be compiled into a library with all types we
care about, and we don't have to constantly recompile units.

This means that we are becoming less concerned about maintaining type lists
everywhere. Often we can drop the type list and pass data across libraries.

With that in mind, it makes less sense for `VariantArrayHandle` to actually
be a `using` alias for `VariantArrayHandleBase<VTKM_DEFAULT_TYPE_LIST>`.

In response, we can revert the is-a relationship between the two. Have a
completely typeless version as the base class and have a second version
templated version to express when the type of the array has been partially
narrowed down to given type lists.

### New Name and Structure

The ultimate purpose of this class is to store an `ArrayHandle` where the
value and storage types are unknown. Thus, an appropriate name for the
class is `UnknownArrayHandle`.

`UnknownArrayHandle` is _not_ templated. It simply stores an `ArrayHandle`
in a typeless (`void *`) buffer. It does, however, contain many templated
methods that allow you to query whether the contained array matches given
types, to cast to given types, and to cast and call to a given functor
(from either given type lists or default lists).

Rather than have a virtual class structure to manage the typeless array,
the new management will use function pointers. This has shown to sometimes
improve compile times and generate less code.

Sometimes it is the case that the set of potential types can be narrowed. In
this case, the array ceases to be unknown and becomes _uncertain_. Thus,
the companion class to `UnknownArrayHandle` is `UncertainArrayHandle`.

`UncertainArrayHandle` has two template parameters: a list of potential
value types and a list of potential storage types. The behavior of
`UncertainArrayHandle` matches that of `UnknownArrayHandle` (and might
inherit from it). However, for `CastAndCall` operations, it will use the
type lists defined in its template parameters.

### Serializing UnknownArrayHandle

Because `UnknownArrayHandle` is not templated, it contains some
opportunities to compile things into the `vtkm_cont` library. Templated
methods like `CastAndCall` cannot be, but the specializations of DIY's
serialize can be.

And since it only has to be compiled once into a library, we can spend some
extra time compiling for more types. We don't have to restrict ourselves to
`VTKM_DEFAULT_TYPE_LIST`. We can compile for vtkm::TypeListTagAll.


Support `ArrayHandleSOA` as a "default" array

Many programs, particularly simulations, store fields of vectors in
separate arrays for each component. This maps to the storage of
`ArrayHandleSOA`. The VTK-m code tends to prefer the AOS storage (which is
what is implemented in `ArrayHandleBasic`, and the behavior of which is
inherited from VTK). VTK-m should better support adding `ArrayHandleSOA` as
one of the types.

We now have a set of default types for Ascent that uses SOA as one of the
basic types.

Part of this change includes an intentional feature regression of
`ArrayHandleSOA` to only support value types of `Vec`. Previously, scalar
types were supported. However, the behavior of `ArrayHandleSOA` is exactly
the same as `ArrayHandleBasic`, except a lot more template code has to be
generated. That itself is not a huge deal, but because you have 2 types
that essentially do the same thing, a lot of template code in VTK-m would
unwind to create two separate code paths that do the same thing with the
same data. To avoid creating those code paths, we simply make any use of
`ArrayHandleSOA` without a `Vec` value invalid. This will prevent VTK-m
from creating those code paths.


## Removed old `ArrayHandle` transfer mechanism

Deleted the default implementation of `ArrayTransfer`. `ArrayTransfer` is
used with the old `ArrayHandle` style to move data between host and device.
The new version of `ArrayHandle` does not use `ArrayTransfer` at all
because this functionality is wrapped in `Buffer` (where it can exist in a
precompiled library).

Once all the old `ArrayHandle` classes are gone, this class will be removed
completely. Although all the remaining `ArrayHandle` classes provide their
own versions of `ArrayTransfer`, they still need the prototype to be
defined to specialize. Thus, the guts of the default `ArrayTransfer` are
removed and replaced with a compile error if you try to compile it.

Also removed `ArrayManagerExecution`. This class was used indirectly by the
old `ArrayHandle`, through `ArrayHandleTransfer`, to move data to and from
a device. This functionality has been replaced in the new `ArrayHandle`s
through the `Buffer` class (which can be compiled into libraries rather
than make every translation unit compile their own template).


## Order asynchronous `ArrayHandle` access

The recent feature of [tokens that scope access to
`ArrayHandle`s](scoping-tokens.md) allows multiple threads to use the same
`ArrayHandle`s without read/write hazards. The intent is twofold. First, it
allows two separate threads in the control environment to independently
schedule tasks. Second, it allows us to move toward scheduling worklets and
other algorithms asynchronously.

However, there was a flaw with the original implementation. Once requests
to an `ArrayHandle` get queued up, they are resolved in arbitrary order.
This might mean that things run in surprising and incorrect order.

### Problematic use case

To demonstrate the flaw in the original implementation, let us consider a
future scenario where when you invoke a worklet (on OpenMP or TBB), the
call to invoke returns immediately and the actual work is scheduled
asynchronously. Now let us say we have a sequence of 3 worklets we wish to
run: `Worklet1`, `Worklet2`, and `Worklet3`. One of `Worklet1`'s parameters
is a `FieldOut` that creates an intermediate `ArrayHandle` that we will
simply call `array`. `Worklet2` is given `array` as a `FieldInOut` to
modify its values. Finally, `Worklet3` is given `array` as a `FieldIn`. It
is clear that for the computation to be correct, the three worklets _must_
execute in the correct order of `Worklet1`, `Worklet2`, and `Worklet3`.

The problem is that if `Worklet2` and `Worklet3` are both scheduled before
`Worklet1` finishes, the order they are executed could be arbitrary. Let us
say that `Worklet1` is invoked, and the invoke call returns before the
execution of `Worklet1` finishes.

The calling code immediately invokes `Worklet2`. Because `array` is already
locked by `Worklet1`, `Worklet2` does not execute right away. Instead, it
waits on a condition variable of `array` until it is free. But even though
the scheduling of `Worklet2` is blocked, the invoke returns because we are
scheduling asynchronously.

Likewise, the calling code then immediately calls invoke for `Worklet3`.
`Worklet3` similarly waits on the condition variable of `array` until it is
free.

Let us assume the likely event that both `Worklet2` and `Worklet3` get
scheduled before `Worklet1` finishes. When `Worklet1` then later does
finish, it's token relinquishes the lock on `array`, which wakes up the
threads waiting for access to `array`. However, there is no imposed order on
in what order the waiting threads will acquire the lock and run. (At least,
I'm not aware of anything imposing an order.) Thus, it is quite possible
that `Worklet3` will wake up first. It will see that `array` is no longer
locked (because `Worklet1` has released it and `Worklet2` has not had a
chance to claim it).

Oops. Now `Worklet3` is operating on `array` before `Worklet2` has had a
chance to put the correct values in it. The results will be wrong.

### Queuing requests

What we want is to impose the restriction that locks to an `ArrayHandle`
get resolved in the order that they are requested. In the previous example,
we have 3 requests on an array that happen in a known order. We want
control given to them in the same order.

To implement this, we need to impose another restriction on the
`condition_variable` when waiting to read or write. We want the lock to go
to the thread that first started waiting. To do this, we added an
internal queue of `Token`s to the `ArrayHandle`.

In `ArrayHandle::WaitToRead` and `ArrayHandle::WaitToWrite`, it first adds
its `Token` to the back of the queue before waiting on the condition
variable. In the `CanRead` and `CanWrite` methods, it checks this queue to
see if the provided `Token` is at the front. If not, then the lock is
denied and the thread must continue to wait.

### Early enqueuing

Another issue that can happen in the previous example is that as threads
are spawned for the 3 different worklets, they may actually start running
in an unexpected order. So the thread running `Worklet3` might actually
start before the other 2 and place itself in the queue first.

The solution is to add a method to `ArrayHandle` called `Enqueue`. This
method takes a `Token` object and adds that `Token` to the queue. However,
regardless of where the `Token` ends up on the queue, the method
immediately returns. It does not attempt to lock the `ArrayHandle`.

So now we can ensure that `Worklet1` properly locks `array` with this
sequence of events. First, the main thread calls `array.Enqueue`. Then a
thread is spawned to call `PrepareForOutput`.

Even if control returns to the calling code and it calls invoke for
`Worklet2` before this spawned thread starts, `Worklet2` cannot start
first. When `PrepareForInput` is called on `array`, it is queued after the
`Token` for `Worklet1`, even if `Worklet1` has not started waiting on the
`array`.


## Improvements to moving data into ArrayHandle

We have made several improvements to adding data into an `ArrayHandle`.

### Moving data from an `std::vector`

For numerous reasons, it is convenient to define data in a `std::vector`
and then wrap that into an `ArrayHandle`. There are two obvious ways to do
this. First, you could deep copy the data into an `ArrayHandle`, which has
obvious drawbacks. Second, you could take the pointer for the data in the
`std::vector` and use that as user-allocated memory in the `ArrayHandle`
without deep copying it. The problem with this shallow copy is that it is
unsafe. If the `std::vector` goes out of scope (or gets resized), then the
data the `ArrayHandle` is pointing to becomes unallocated, which will lead
to unpredictable behavior.

However, there is a third option. It is often the case that an
`std::vector` is filled and then becomes unused once it is converted to an
`ArrayHandle`. In this case, what we really want is to pass the data off to
the `ArrayHandle` so that the `ArrayHandle` is now managing the data and
not the `std::vector`.

C++11 has a mechanism to do this: move semantics. You can now pass
variables to functions as an "rvalue" (right-hand value). When something is
passed as an rvalue, it can pull state out of that variable and move it
somewhere else. `std::vector` implements this movement so that an rvalue
can be moved to another `std::vector` without actually copying the data.
`make_ArrayHandle` now also takes advantage of this feature to move rvalue
`std::vector`s.

There is a special form of `make_ArrayHandle` named `make_ArrayHandleMove`
that takes an rvalue. There is also a special overload of
`make_ArrayHandle` itself that handles an rvalue `vector`. (However, using
the explicit move version is better if you want to make sure the data is
actually moved.)

So if you create the `std::vector` in the call to `make_ArrayHandle`, then
the data only gets created once.

``` cpp
auto array = vtkm::cont::make_ArrayHandleMove(std::vector<vtkm::Id>{ 2, 6, 1, 7, 4, 3, 9 });
```

Note that there is now a better way to express an initializer list to
`ArrayHandle` documented below. But this form of `ArrayHandleMove` can be
particularly useful for initializing an array to all of a particular value.
For example, an easy way to initialize an array of 1000 elements all to 1
is

``` cpp
auto array = vtkm::cont::make_ArrayHandleMove(std::vector<vtkm::Id>(1000, 1));
```

You can also move the data from an already created `std::vector` by using
the `std::move` function to convert it to an rvalue. When you do this, the
`std::vector` becomes invalid after the call and any use will be undefined.

``` cpp
std::vector<vtkm::Id> vector;
// fill vector

auto array = vtkm::cont::make_ArrayHandleMove(std::move(vector));
```

### Make `ArrayHandle` from initalizer list

A common use case for using `std::vector` (particularly in our unit tests)
is to quickly add an initalizer list into an `ArrayHandle`. Repeating the
example from above:

``` cpp
auto array = vtkm::cont::make_ArrayHandleMove(std::vector<vtkm::Id>{ 2, 6, 1, 7, 4, 3, 9 });
```

However, creating the `std::vector` should be unnecessary. Why not be able
to create the `ArrayHandle` directly from an initializer list? Now you can
by simply passing an initializer list to `make_ArrayHandle`.

``` cpp
auto array = vtkm::cont::make_ArrayHandle({ 2, 6, 1, 7, 4, 3, 9 });
```

There is an issue here. The type here can be a little ambiguous (for
humans). In this case, `array` will be of type
`vtkm::cont::ArrayHandleBasic<int>`, since that is what an integer literal
defaults to. This could be a problem if, for example, you want to use
`array` as an array of `vtkm::Id`, which could be of type `vtkm::Int64`.
This is easily remedied by specifying the desired value type as a template
argument to `make_ArrayHandle`.

``` cpp
auto array = vtkm::cont::make_ArrayHandle<vtkm::Id>({ 2, 6, 1, 7, 4, 3, 9 });
```

### Deprecated `make_ArrayHandle` with default shallow copy

For historical reasons, passing an `std::vector` or a pointer to
`make_ArrayHandle` does a shallow copy (i.e. `CopyFlag` defaults to `Off`).
Although more efficient, this mode is inherintly unsafe, and making it the
default is asking for trouble.

To combat this, calling `make_ArrayHandle` without a copy flag is
deprecated. In this way, if you wish to do the faster but more unsafe
creation of an `ArrayHandle` you should explicitly express that.

This requried quite a few changes through the VTK-m source (particularly in
the tests).

### Similar changes to `Field`

`vtkm::cont::Field` has a `make_Field` helper function that is similar to
`make_ArrayHandle`. It also features the ability to create fields from
`std::vector`s and C arrays. It also likewise had the same unsafe behavior
by default of not copying from the source of the arrays.

That behavior has similarly been depreciated. You now have to specify a
copy flag.

The ability to construct a `Field` from an initializer list of values has
also been added.


## Deprecate ArrayHandleVirtualCoordinates

As we port VTK-m to more types of accelerator architectures, supporting
virtual methods is becoming more problematic. Thus, we are working to back
out of using virtual methods in the execution environment.

One of the most widespread users of virtual methods in the execution
environment is `ArrayHandleVirtual`. As a first step of deprecating this
class, we first deprecate the `ArrayHandleVirtualCoordinates` subclass.

Not surprisingly, `ArrayHandleVirtualCoordinates` is used directly by
`CoordinateSystem`. The biggest change necessary was that the `GetData`
method returned an `ArrayHandleVirtualCoordinates`, which obviously would
not work if that class is deprecated.

An oddness about this return type is that it is quite different from the
superclass's method of the same name. Rather, `Field` returns a
`VariantArrayHandle`. Since this had to be corrected anyway, it was decided
to change `CoordinateSystem`'s `GetData` to also return a
`VariantArrayHandle`, although its typelist is set to just `vtkm::Vec3f`.

To try to still support old code that expects the deprecated behavior of
returning an `ArrayHandleVirtualCoordinates`, `CoordinateSystem::GetData`
actually returns a "hidden" subclass of `VariantArrayHandle` that
automatically converts itself to an `ArrayHandleVirtualCoordinates`. (A
deprecation warning is given if this is done.)

This approach to support deprecated code is not perfect. The returned value
for `CoordinateSystem::GetData` can only be used as an `ArrayHandle` if a
method is directly called on it or if it is cast specifically to
`ArrayHandleVirtualCoordinates` or its superclass. For example, if passing
it to a method argument typed as `vtkm::cont::ArrayHandle<T, S>` where `T`
and `S` are template parameters, then the conversion will fail.

To continue to support ease of use, `CoordinateSystem` now has a method
named `GetDataAsMultiplexer` that returns the data as an
`ArrayHandleMultiplexer`. This can be employed to quickly use the
`CoordinateSystem` as an array without the overhead of a `CastAndCall`.

## ArrayHandleDecorator Allocate and Shrink Support

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


## Portals may advertise custom iterators

The `ArrayPortalToIterator` utilities are used to produce STL-style iterators 
from vtk-m's `ArrayHandle` portals. By default, a facade class is constructed 
around the portal API, adapting it to an iterator interface. 

However, some portals use iterators internally, or may be able to construct a
lightweight iterator easily. For these, it is preferable to directly use the
specialized iterators instead of going through the generic facade. A portal may
now declare the following optional API to advertise that it has custom 
iterators:

```
struct MyPortal
{
  using IteratorType = ...; // alias to the portal's specialized iterator type
  IteratorType GetIteratorBegin(); // Return the begin iterator
  IteratorType GetIteratorEnd(); // Return the end iterator

  // ...rest of ArrayPortal API...
};
```

If these members are present, `ArrayPortalToIterators` will forward the portal's
specialized iterators instead of constructing a facade. This works when using 
the `ArrayPortalToIterators` class directly, and also with the
`ArrayPortalToIteratorBegin` and `ArrayPortalToIteratorEnd` convenience 
functions.


## Redesign of ArrayHandle to access data using typeless buffers

The original implementation of `ArrayHandle` is meant to be very generic.
To define an `ArrayHandle`, you actually create a `Storage` class that
maintains the data and provides portals to access it (on the host). Because
the `Storage` can provide any type of data structure it wants, you also
need to define an `ArrayTransfer` that describes how to move the
`ArrayHandle` to and from a device. It also has to be repeated for every
translation unit that uses them.

This is a very powerful mechanism. However, one of the major problems with
this approach is that every `ArrayHandle` type needs to have a separate
compile path for every value type crossed with every device. Because of
this limitation, the `ArrayHandle` for the basic storage has a special
implementation that manages the actual data allocation and movement as
`void *` arrays. In this way all the data management can be compiled once
and put into the `vtkm_cont` library. This has dramatically improved the
VTK-m compile time.

This new design replicates the basic `ArrayHandle`'s success to all other
storage types. The basic idea is to make the implementation of
`ArrayHandle` storage slightly less generic. Instead of requiring it to
manage the data it stores, it instead just builds `ArrayPortal`s from
`void` pointers that it is given. The management of `void` pointers can be
done in non-templated classes that are compiled into a library.

This initial implementation does not convert all `ArrayHandle`s to avoid
making non-backward compatible changes before the next minor revision of
VTK-m. In particular, it would be particularly difficult to convert
`ArrayHandleVirtual`. It could be done, but it would be a lot of work for a
class that will likely be removed.

### Buffer

Key to these changes is the introduction of a
`vtkm::cont::internal::Buffer` object. As the name implies, the `Buffer`
object manages a single block of bytes. `Buffer` is agnostic to the type of
data being stored. It only knows the length of the buffer in bytes. It is
responsible for allocating space on the host and any devices as necessary
and for transferring data among them. (Since `Buffer` knows nothing about
the type of data, a precondition of VTK-m would be that the host and all
devices have to have the same endian.)

The idea of the `Buffer` object is similar in nature to the existing
`vtkm::cont::internal::ExecutionArrayInterfaceBasicBase` except that it
will manage a buffer of data among the control and all devices rather than
in one device through a templated subclass.

As explained below, `ArrayHandle` holds some fixed number of `Buffer`
objects. (The number can be zero for implicit `ArrayHandle`s.) Because all
the interaction with the devices happen through `Buffer`, it will no longer
be necessary to compile any reference to `ArrayHandle` for devices (e.g.
you won't have to use nvcc just because the code links `ArrayHandle.h`).

### Storage

The `vtkm::cont::internal::Storage` class changes dramatically. Although an
instance will be kept, the intention is for `Storage` itself to be a
stateless object. It will manage its data through `Buffer` objects provided
from the `ArrayHandle`.

That said, it is possible for `Storage` to have some state. For example,
the `Storage` for `ArrayHandleImplicit` must hold on to the instance of the
portal used to manage the state.


### ArrayTransport

The `vtkm::cont::internal::ArrayTransfer` class will be removed completely.
All data transfers will be handled internally with the `Buffer` object

### Portals

A big change for this design is that the type of a portal for an
`ArrayHandle` will be the same for all devices and the host. Thus, we no
longer need specialized versions of portals for each device. We only have
one portal type. And since they are constructed from `void *` pointers, one
method can create them all.


### Advantages

The `ArrayHandle` interface should not change significantly for external
uses, but this redesign offers several advantages.

#### Faster Compiles

Because the memory management is contained in a non-templated `Buffer`
class, it can be compiled once in a library and used by all template
instances of `ArrayHandle`. It should have similar compile advantages to
our current specialization of the basic `ArrayHandle`, but applied to all
types of `ArrayHandle`s.

#### Fewer Templates

Hand-in-hand with faster compiles, the new design should require fewer
templates and template instances. We have immediately gotten rid of
`ArrayTransport`. `Storage` is also much shorter. Because all
`ArrayPortal`s are the same for every device and the host, we need many
fewer versions of those classes. In the device adapter, we can probably
collapse the three `ArrayManagerExecution` classes into a single, much
simpler class that does simple memory allocation and copy.

#### Fewer files need to be compiled for CUDA

Including `ArrayHandle.h` no longer adds code that compiles for a device.
Thus, we should no longer need to compile for a specific device adapter
just because we access an `ArrayHandle`. This should make it much easier to
achieve our goal of a "firewall". That is, code that just calls VTK-m
filters does not need to support all its compilers and flags.

#### Simpler ArrayHandle specialization

The newer code should simplify the implementation of special `ArrayHandle`s
a bit. You need only implement an `ArrayPortal` that operates on one or
more `void *` arrays and a simple `Storage` class.

#### Out of band memory sharing

With the current version of `ArrayHandle`, if you want to take data from
one `ArrayHandle` you pretty much have to create a special template to wrap
another `ArrayHandle` around that. With this new design, it is possible to
take data from one `ArrayHandle` and give it to another `ArrayHandle` of a
completely different type. You can't do this willy-nilly since different
`ArrayHandle` types will interpret buffers differently. But there can be
some special important use cases.

One such case could be an `ArrayHandle` that provides strided access to a
buffer. (Let's call it `ArrayHandleStride`.) The idea is that it interprets
the buffer as an array for a particular type (like a basic `ArrayHandle`)
but also defines a stride, skip, and repeat so that given an index it looks
up the value `((index / skip) % repeat) * stride`. The point is that it can
take an AoS array of tuples and represent an array of one of the
components.

The point would be that if you had a `VariantArrayHandle` or `Field`, you
could pull out an array of one of the components as an `ArrayHandleStride`.
An `ArrayHandleStride<vtkm::Float32>` could be used to represent that data
that comes from any basic `ArrayHandle` with `vtkm::Float32` or a
`vtkm::Vec` of that type. It could also represent data from an
`ArrayHandleCartesianProduct` and `ArrayHandleSoA`. We could even represent
an `ArrayHandleUniformPointCoordinates` by just making a small array. This
allows us to statically access a whole bunch of potential array storage
classes with a single type.

#### Potentially faster device transfers

There is currently a fast-path for basic `ArrayHandle`s that does a block
cuda memcpy between host and device. But for other `ArrayHandle`s that do
not defer their `ArrayTransfer` to a sub-array, the transfer first has to
copy the data into a known buffer.

Because this new design stores all data in `Buffer` objects, any of these
can be easily and efficiently copied between devices.

### Disadvantages

This new design gives up some features of the original `ArrayHandle` design.

#### Can only interface data that can be represented in a fixed number of buffers

Because the original `ArrayHandle` design required the `Storage` to
completely manage the data, it could represent it in any way possible. In
this redesign, the data need to be stored in some fixed number of memory
buffers.

This is a pretty open requirement. I suspect most data formats will be
storable in this. The user's guide has an example of data stored in a
`std::deque` that will not be representable. But that is probably not a
particularly practical example.

#### VTK-m would only be able to support hosts and devices with the same endian

Because data are transferred as `void *` blocks of memory, there is no way
to correct words if the endian on the two devices does not agree. As far as
I know, there should be no issues with the proposed ECP machines.

If endian becomes an issue, it might be possible to specify a word length
in the `Buffer`. That would assume that all numbers stored in the `Buffer`
have the same word length.

#### ArrayPortals must be completely recompiled in each translation unit

We can declare that an `ArrayHandle` does not need to include the device
adapter header files in part because it no longer needs specialized
`ArrayPortal`s for each device. However, that means that a translation unit
compiled with the host compiler (say gcc) will produce different code for
the `ArrayPortal`s than those with the device compiler (say nvcc). This
could lead to numerous linking problems.

To get around these issues, we will probably have to enforce no exporting
of any of the `ArrayPotal` symbols and force them all to be recompiled for
each translation unit. This will serve to increase the compile times a bit.
We will probably also still encounter linking errors as there would be no
way to enforce this requirement.

#### Cannot have specialized portals for the control environment

Because the new design unifies `ArrayPortal` types across control and
execution environments, it is no longer possible to have a special version
for the control environment to manage resources. This will require removing
some recent behavior of control portals such as with MR !1988.


## `ArrayRangeCompute` works on any array type without compiling device code

Originally, `ArrayRangeCompute` required you to know specifically the
`ArrayHandle` type (value type and storage type) and to compile using any
device compiler. The method is changed to include only overloads that have
precompiled versions of `ArrayRangeCompute`.

Additionally, an `ArrayRangeCompute` overload that takes an
`UnknownArrayHandle` has been added. In addition to allowing you to compute
the range of arrays of unknown types, this implementation of
`ArrayRangeCompute` serves as a fallback for `ArrayHandle` types that are
not otherwise explicitly supported.

If you really want to make sure that you compute the range directly on an
`ArrayHandle` of a particular type, you can include
`ArrayRangeComputeTemplate.h`, which contains a templated overload of
`ArrayRangeCompute` that directly computes the range of an `ArrayHandle`.
Including this header requires compiling for device code.


## Implemented ArrayHandleRandomUniformBits and ArrayHandleRandomUniformReal

ArrayHandleRandomUniformBits and ArrayHandleRandomUniformReal were added to provide
an efficient way to generate pseudo random numbers in parallel. They are based on the
Philox parallel pseudo random number generator. ArrayHandleRandomUniformBits provides
64-bits random bits in the whole range of UInt64 as its content while
ArrayHandleRandomUniformReal provides random Float64 in the range of [0, 1). User can
either provide a seed in the form of Vec<vtkm::Uint32, 1> or use the default random
source provided by the C++ standard library. Both of the ArrayHandles  are lazy evaluated
as other Fancy ArrayHandles such that they only have O(1) memory overhead. They are
stateless and functional and does not change once constructed. To generate a new set of
random numbers, for example as part of a iterative algorithm, a  new ArrayHandle
needs to be constructed in each iteration. See the user's guide for more detail and
examples.


## Extract component arrays from unknown arrays

One of the problems with the data structures of VTK-m is that non-templated
classes like `DataSet`, `Field`, and `UnknownArrayHandle` (formally
`VariantArrayHandle`) internally hold an `ArrayHandle` of a particular type
that has to be cast to the correct task before it can be reasonably used.
That in turn is problematic because the list of possible `ArrayHandle`
types is very long.

At one time we were trying to compensate for this by using
`ArrayHandleVirtual`. However, for technical reasons this class is
infeasible for every use case of VTK-m and has been deprecated. Also, this
was only a partial solution since using it still required different code
paths for, say, handling values of `vtkm::Float32` and `vtkm::Vec3f_32`
even though both are essentially arrays of 32-bit floats.

The extract component feature compensates for this problem by allowing you
to extract the components from an `ArrayHandle`. This feature allows you to
create a single code path to handle `ArrayHandle`s containing scalars or
vectors of any size. Furthermore, when you extract a component from an
array, the storage gets normalized so that one code path covers all storage
types.

### `ArrayExtractComponent`

The basic enabling feature is a new function named `ArrayExtractComponent`.
This function takes takes an `ArrayHandle` and an index to a component. It
then returns an `ArrayHandleStride` holding the selected component of each
entry in the original array.

We will get to the structure of `ArrayHandleStride` later. But the
important part is that `ArrayHandleStride` does _not_ depend on the storage
type of the original `ArrayHandle`. That means whether you extract a
component from `ArrayHandleBasic`, `ArrayHandleSOA`,
`ArrayHandleCartesianProduct`, or any other type, you get back the same
`ArrayHandleStride`. Likewise, regardless of whether the input
`ArrayHandle` has a `ValueType` of `FloatDefault`, `Vec2f`, `Vec3f`, or any
other `Vec` of a default float, you get the same `ArrayHandleStride`. Thus,
you can see how this feature can dramatically reduce code paths if used
correctly.

It should be noted that `ArrayExtractComponent` will (logically) flatten
the `ValueType` before extracting the component. Thus, nested `Vec`s such
as `Vec<Vec3f, 3>` will be treated as a `Vec<FloatDefault, 9>`. The
intention is so that the extracted component will always be a basic C type.
For the purposes of this document when we refer to the "component type", we
really mean the base component type.

Different `ArrayHandle` implementations provide their own implementations
for `ArrayExtractComponent` so that the component can be extracted without
deep copying all the data. We will visit how `ArrayHandleStride` can
represent different data layouts later, but first let's go into the main
use case.

### Extract components from `UnknownArrayHandle`

The principle use case for `ArrayExtractComponent` is to get an
`ArrayHandle` from an unknown array handle without iterating over _every_
possible type. (Rather, we iterate over a smaller set of types.) To
facilitate this, an `ExtractComponent` method has been added to
`UnknownArrayHandle`.

To use `UnknownArrayHandle::ExtractComponent`, you must give it the
component type. You can check for the correct component type by using the
`IsBaseComponentType` method. The method will then return an
`ArrayHandleStride` for the component type specified.

#### Example

As an example, let's say you have a worklet, `FooWorklet`, that does some
per component operation on an array. Furthermore, let's say that you want
to implement a function that, to the best of your ability, can apply
`FooWorklet` on an array of any type. This function should be pre-compiled
into a library so it doesn't have to be compiled over and over again.
(`MapFieldPermutation` and `MapFieldMergeAverage` are real and important
examples that have this behavior.)

Without the extract component feature, the implementation might look
something like this (many practical details left out):

``` cpp
struct ApplyFooFunctor
{
  template <typename ArrayType>
  void operator()(const ArrayType& input, vtkm::cont::UnknownArrayHandle& output) const
  {
    ArrayType outputArray;
	vtkm::cont::Invoke invoke;
	invoke(FooWorklet{}, input, outputArray);
	output = outputArray;
  }
};

vtkm::cont::UnknownArrayHandle ApplyFoo(const vtkm::cont::UnknownArrayHandle& input)
{
  vtkm::cont::UnknownArrayHandle output;
  input.CastAndCallForTypes<vtkm::TypeListAll, VTKM_DEFAULT_STORAGE_LIST_TAG>(
    ApplyFooFunctor{}, output);
  return output;
}
```

Take a look specifically at the `CastAndCallForTypes` call near the bottom
of this example. It calls for all types in `vtkm::TypeListAll`, which is
about 40 instances. Then, it needs to be called for any type in the desired
storage list. This could include basic arrays, SOA arrays, and lots of
other specialized types. It would be expected for this code to generate
over 100 paths for `ApplyFooFunctor`. This in turn contains a worklet
invoke, which is not a small amount of code.

Now consider how we can use the `ExtractComponent` feature to reduce the
code paths:

``` cpp
struct ApplyFooFunctor
{
  template <typename T>
  void operator()(T,
                  const vtkm::cont::UnknownArrayHandle& input, 
				  cont vtkm::cont::UnknownArrayHandle& output) const
  {
    if (!input.IsBasicComponentType<T>()) { return; }
	VTKM_ASSERT(output.IsBasicComponentType<T>());

	vtkm::cont::Invoke invoke;
	invoke(FooWorklet{}, input.ExtractComponent<T>(), output.ExtractComponent<T>());
  }
};

vtkm::cont::UnknownArrayHandle ApplyFoo(const vtkm::cont::UnknownArrayHandle& input)
{
  vtkm::cont::UnknownArrayHandle output = input.NewInstanceBasic();
  output.Allocate(input.GetNumberOfValues());
  vtkm::cont::ListForEach(ApplyFooFunctor{}, vtkm::TypeListScalarAll{}, input, output);
  return output;
}
```

The number of lines of code is about the same, but take a look at the
`ListForEach` (which replaces the `CastAndCallForTypes`). This calling code
takes `TypeListScalarAll` instead of `TypeListAll`, which reduces the
instances created from around 40 to 13 (every basic C type). It is also no
longer dependent on the storage, so these 13 instances are it. As an
example of potential compile savings, changing the implementation of the
`MapFieldMergePermutation` and `MapFieldMergeAverage` functions in this way
reduced the filters_common library (on Mac, Debug build) by 24 MB (over a
third of the total size).

Another great advantage of this approach is that even though it takes less
time to compile and generates less code, it actually covers more cases.
Have an array containg values of `Vec<short, 13>`? No problem. The values
were actually stored in an `ArrayHandleReverse`? It will still work.

### `ArrayHandleStride`

This functionality is made possible with the new `ArrayHandleStride`. This
array behaves much like `ArrayHandleBasic`, except that it contains an
_offset_ parameter to specify where in the buffer array to start reading
and a _stride_ parameter to specify how many entries to skip for each
successive entry. `ArrayHandleStride` also has optional parameters
`divisor` and `modulo` that allow indices to be repeated at regular
intervals.

Here are how `ArrayHandleStride` extracts components from several common
arrays. For each of these examples, we assume that the `ValueType` of the
array is `Vec<T, N>`. They are each extracting _component_.

#### Extracting from `ArrayHandleBasic`

When extracting from an `ArrayHandleBasic`, we just need to start at the
proper component and skip the length of the `Vec`.

* _offset_: _component_
* _stride_: `N`

#### Extracting from `ArrayHandleSOA`

Since each component is held in a separate array, they are densly packed.
Each component could be represented by `ArrayHandleBasic`, but of course we
use `ArrayHandleStride` to keep the type consistent.

* _offset_: 0
* _stride_: 1

#### Extracting from `ArrayHandleCartesianProduct`

This array is the basic reason for implementing the _divisor_ and _modulo_
parameters. Each of the 3 components have different parameters, which are
the following (given that _dims_[3] captures the size of the 3 arrays for
each dimension).

* _offset_: 0
* _stride_: 1
* case _component_ == 0
  * _divisor_: _ignored_
  * _modulo_: _dims_[0]
* case _component_ == 1
  * _divisor_: _dims_[0]
  * _modulo_: _dims_[1]
* case _component_ == 2
  * _divisor_: _dims_[0]
  * _modulo_: _ignored_

#### Extracting from `ArrayHandleUniformPointCoordinates`

This array cannot be represented directly because it is fully implicit.
However, it can be trivially converted to `ArrayHandleCartesianProduct` in
typically very little memory. (In fact, EAVL always represented uniform
point coordinates by explicitly storing a Cartesian product.) Thus, for
very little overhead the `ArrayHandleStride` can be created.

### Runtime overhead of extracting components

These benefits come at a cost, but not a large one. The "biggest" cost is
the small cost of computing index arithmetic for each access into
`ArrayHandleStride`. To make this as efficient as possible, there are
conditions that skip over the modulo and divide steps if they are not
necessary. (Integer modulo and divide tend to take much longer than
addition and multiplication.) It is for this reason that we probably do not
want to use this method all the time.

Another cost is the fact that not every `ArrayHandle` can be represented by
`ArrayHandleStride` directly without copying. If you ask to extract a
component that cannot be directly represented, it will be copied into a
basic array, which is not great. To make matters worse, for technical
reasons this copy happens on the host rather than the device.


## `ArrayHandleGroupVecVariable` holds now one more offset.

This change affects the usage of both `ConvertNumComponentsToOffsets` and
 `make_ArrayHandleGroupVecVariable`.

The reason of this change is to remove a branch in
`ArrayHandleGroupVecVariable::Get` which is used to avoid an array overflow, 
this in theory would increases the performance since at the CPU level it will 
remove penalties due to wrong branch predictions.

The change affects `ConvertNumComponentsToOffsets` by both:

 1. Increasing the numbers of elements in `offsetsArray` (its second parameter) 
    by one.

 2. Setting `sourceArraySize` as the sum of all the elements plus the new one 
    in `offsetsArray`

Note that not every specialization of `ConvertNumComponentsToOffsets` does
return `offsetsArray`. Thus, some of them would not be affected.

Similarly, this change affects `make_ArrayHandleGroupVecVariable` since it
expects its second parameter (offsetsArray) to be one element bigger than
before.

# Control Environment

## Algorithms for Control and Execution Environments

The `<vtkm/Algorithms.h>` header has been added to provide common STL-style 
generic algorithms that are suitable for use in both the control and execution 
environments. This is necessary as the STL algorithms in the `<algorithm>` 
header are not marked up for use in execution environments such as CUDA. 

In addition to the markup, these algorithms have convenience overloads to 
support ArrayPortals directly, simplifying their usage with VTK-m data 
structures.

Currently, three related algorithms are provided: `LowerBounds`, `UpperBounds`,
and `BinarySearch`. `BinarySearch` differs from the STL `std::binary_search` 
algorithm in that it returns an iterator (or index) to a matching element,
rather than just a boolean indicating whether a or not key is present.

The new algorithm signatures are:

```c++
namespace vtkm
{

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT 
IterT BinarySearch(IterT first, IterT last, const T& val, Comp comp);

template <typename IterT, typename T>
VTKM_EXEC_CONT 
IterT BinarySearch(IterT first, IterT last, const T& val);

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT 
vtkm::Id BinarySearch(const PortalT& portal, const T& val, Comp comp);

template <typename PortalT, typename T>
VTKM_EXEC_CONT 
vtkm::Id BinarySearch(const PortalT& portal, const T& val);

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT 
IterT LowerBound(IterT first, IterT last, const T& val, Comp comp);

template <typename IterT, typename T>
VTKM_EXEC_CONT 
IterT LowerBound(IterT first, IterT last, const T& val);

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT 
vtkm::Id LowerBound(const PortalT& portal, const T& val, Comp comp);

template <typename PortalT, typename T>
VTKM_EXEC_CONT 
vtkm::Id LowerBound(const PortalT& portal, const T& val);

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT 
IterT UpperBound(IterT first, IterT last, const T& val, Comp comp);

template <typename IterT, typename T>
VTKM_EXEC_CONT 
IterT UpperBound(IterT first, IterT last, const T& val);

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT 
vtkm::Id UpperBound(const PortalT& portal, const T& val, Comp comp);

template <typename PortalT, typename T>
VTKM_EXEC_CONT 
vtkm::Id UpperBound(const PortalT& portal, const T& val);

}
```

# Execution Environment

## Scope ExecObjects with Tokens

When VTK-m's `ArrayHandle` was originally designed, it was assumed that the
control environment would run on a single thread. However, multiple users
have expressed realistic use cases in which they would like to control
VTK-m from multiple threads (for example, to control multiple devices).
Consequently, it is important that VTK-m's control classes work correctly
when used simultaneously from multiple threads.

The original `PrepareFor*` methods of `ArrayHandle` returned an object to
be used in the execution environment on a particular device that pointed to
data in the array. The pointer to the data was contingent on the state of
the `ArrayHandle` not changing. The assumption was that the calling code
would immediately use the returned execution environment object and would
not further change the `ArrayHandle` until done with the execution
environment object.

This assumption is broken if multiple threads are running in the control
environment. For example, if one thread has called `PrepareForInput` to get
an execution array portal, the portal or its data could become invalid if
another thread calls `PrepareForOutput` on the same array. Initially one
would think that a well designed program should not share `ArrayHandle`s in
this way, but there are good reasons to need to do so. For example, when
using `vtkm::cont::PartitionedDataSet` where multiple partitions share a
coordinate system (very common), it becomes unsafe to work on multiple
blocks in parallel on different devices.

What we really want is the code to be able to specify more explicitly when
the execution object is in use. Ideally, the execution object itself would
maintain the resources it is using. However, that will not work in this
case since the object has to pass from control to execution environment and
back. The resource allocation will break when the object is passed to an
offloaded device and back.

Because we cannot use the object itself to manage its own resources, we use
a proxy object we are calling a `Token`. The `Token` object manages the
scope of the return execution object. As long as the `Token` is still in
scope, the execution object will remain valid. When the `Token` is
destroyed (or `DetachFromAll` is called on it), then the execution object
is no longer protected.

When a `Token` is attached to an `ArrayHandle` to protect an execution
object, it's read or write mode is recorded. Multiple `Token`s can be
attached to read the `ArrayHandle` at the same time. However, only one
`Token` can be used to write to the `ArrayHandle`.

### Basic `ArrayHandle` use

The basic use of the `PrepareFor*` methods of `ArrayHandle` remain the
same. The only difference is the addition of a `Token` parameter.

``` cpp
template <typename Device>
void LowLevelArray(vtkm::cont::ArrayHandle<vtkm::Float32> array, Device)
{
  vtkm::cont::Token token;
  auto portal = array.PrepareForOutput(ARRAY_SIZE, Device{}, token);
  // At this point, array is locked from anyone else from reading or modifying
  vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(MyKernel(portal), ARRAY_SIZE);

  // When the function finishes, token goes out of scope and array opens up
  // for other uses.
}
```

### Execution objects

To make sure that execution objects are scoped correctly, many changes
needed to be made to propagate a `Token` reference from the top of the
scope to where the execution object is actually made. The most noticeable
place for this was for implementations of
`vtkm::cont::ExecutionObjectBase`. Most implementations of
`ExecutionObjectBase` create an object that requires data from an
`ArrayHandle`.

Previously, a subclass of `ExecutionObjectBase` was expected to have a
method named `PrepareForExecution` that had a single argument: the device
tag (or id) to make an object for. Now, subclasses of `ExecutionObjectBase`
should have a `PrepareForExecution` that takes two arguments: the device
and a `Token` to use for scoping the execution object.

``` cpp
struct MyExecObject : vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::ArrayHandle<vtkm::Float32> Array;
  
  template <typename Device>
  VTKM_CONT
  MyExec<Device> PrepareForExecution(Device device, vtkm::cont::Token& token)
  {
    MyExec<Device> object;
	object.Portal = this->Array.PrepareForInput(device, token);
	return object;
  }
};
```

It actually still works to use the old style of `PrepareForExecution`.
However, you will get a deprecation warning (on supported compilers) when
you try to use it.

### Invoke and Dispatcher

The `Dispatcher` classes now internally define a `Token` object during the
call to `Invoke`. (Likewise, `Invoker` will have a `Token` defined during
its invoke.) This internal `Token` is used when preparing `ArrayHandle`s
and `ExecutionObject`s for the execution environment. (Details in the next
section on how that works.)

Because the invoke uses a `Token` to protect its arguments, it will block
the execution of other worklets attempting to access arrays in a way that
could cause read-write hazards. In the following example, the second
worklet will not be able to execute until the first worklet finishes.

``` cpp
vtkm::cont::Invoker invoke;
invoke(Worklet1{}, input, intermediate);
invoke(Worklet2{}, intermediate, output); // Will not execute until Worklet1 finishes.
```

That said, invocations _can_ share arrays if their use will not cause
read-write hazards. In particular, two invocations can both use the same
array if they are both strictly reading from it. In the following example,
both worklets can potentially execute at the same time.

``` cpp
vtkm::cont::Invoker invoke;
invoke(Worklet1{}, input, output1);
invoke(Worklet2{}, input, output2); // Will not block
```

The same `Token` is used for all arguments to the `Worklet`. This deatil is
important to prevent deadlocks if the same object is used in more than one
`Worklet` parameter. As a simple example, if a `Worklet` has a control
signature like

``` cpp
  using ControlSignature = void(FieldIn, FieldOut);
```

it should continue to work to use the same array as both fields.

``` cpp
vtkm::cont::Invoker invoke;
invoke(Worklet1{}, array, array);
```

### Transport

The dispatch mechanism of worklets internally uses
`vtkm::cont::arg::Transport` objects to automatically move data from the
control environment to the execution environment. These `Transport` object
now take a `Token` when doing the transportation. This all happens under
the covers for most users.

### Control Portals

The `GetPortalConstControl` and `GetPortalControl` methods have been
deprecated. Instead, the methods `ReadPortal` and `WritePortal` should be
used. The calling signature is the same as their predecessors, but the
returned portal contains a reference back to the original `ArrayHandle`.
The reference keeps track of whether the memory allocation has changed.

If the `ArrayHandle` is changed while the `ArrayPortal` still exists,
nothing will happen immediately. However, if the portal is subsequently
accessed (i.e. `Set` or `Get` is called on it), then a fatal error will be
reported to the log.

### Deadlocks

Now that portal objects from `ArrayHandle`s have finite scope (as opposed
to able to be immediately invalidated), the scopes have the ability to
cause operations to block. This can cause issues if the `ArrayHandle` is
attempted to be used by multiple `Token`s at once.

The following is a contrived example of causing a deadlock.

``` cpp
vtkm::cont::Token token1;
auto portal1 = array.PrepareForInPlace(Device{}, token1);

vtkm::cont::Token token2;
auto portal2 = array.PrepareForInput(Device{}, token2);
```

The last line will deadlock as `PrepareForInput` waits for `token1` to
detach, which will never happen. To prevent this from happening, if you use
the same `Token` on the array, it will always allow the action. Thus, the
following will work fine.

``` cpp
vtkm::cont::Token token;

auto portal1 = array.PrepareForInPlace(Device{}, token);
auto portal2 = array.PrepareForInput(Device{}, token);
```

This prevents deadlock during the invocation of a worklet (so long as no
intermediate object tries to create its own `Token`, which would be bad
practice).

Deadlocks are more likely when actually running multiple threads in the
control environment, but still pretty unlikely. One way it can occur is if
you have one (or more) worklet that has two output fields. You then try to
run the worklet(s) simultaneously on multiple threads. It could be that one
thread locks the first output array and the other thread locks the second
output array.

However, having multiple threads trying to write to the same output arrays
at the same time without its own coordination is probably a bad idea in itself.


## Masks and Scatters Supported for 3D Scheduling

Previous to this change worklets that wanted to use non-default
`vtkm::worklet::Mask` or `vtkm::worklet::Scatter` wouldn't work when scheduled
to run across `vtkm::cont::CellSetStructured` or other `InputDomains` that
supported 3D scheduling.

This restriction was an inadvertent limitation of the VTK-m worklet scheduling
algorithm. Lifting the restriction and providing sufficient information has
been achieved in a manner that shouldn't degrade performance of any existing
worklets.


## Virtual methods in execution environment deprecated

The use of classes with any virtual methods in the execution environment is
deprecated. Although we had code to correctly build virtual methods on some
devices such as CUDA, this feature was not universally supported on all
programming models we wish to support. Plus, the implementation of virtual
methods is not hugely convenient on CUDA because the virtual methods could
not be embedded in a library. To get around virtual methods declared in
different libraries, all builds had to be static, and a special linking
step to pull in possible virtual method implementations was required.

For these reasons, VTK-m is no longer relying on virtual methods. (Other
approaches like multiplexers are used instead.) The code will be officially
removed in version 2.0. It is still supported in a deprecated sense (you
should get a warning). However, if you want to build without virtual
methods, you can set the `VTKm_NO_DEPRECATED_VIRTUAL` CMake flag, and they
will not be compiled.


## Deprecate Execute with policy

The version of `Filter::Execute` that takes a policy as an argument is now
deprecated. Filters are now able to specify their own fields and types,
which is often why you want to customize the policy for an execution. The
other reason is that you are compiling VTK-m into some other source that
uses a particular types of storage. However, there is now a mechanism in
the CMake configuration to allow you to provide a header that customizes
the "default" types used in filters. This is a much more convenient way to
compile filters for specific types.

One thing that filters were not able to do was to customize what cell sets
they allowed using. This allows filters to self-select what types of cell
sets they support (beyond simply just structured or unstructured). To
support this, the lists `SupportedCellSets`, `SupportedStructuredCellSets`,
and `SupportedUnstructuredCellSets` have been added to `Filter`. When you
apply a policy to a cell set, you now have to also provide the filter.

# Worklets and Filters

## Enable setting invalid value in probe filter

Initially, the probe filter would simply not set a value if a sample was
outside the input `DataSet`. This is not great as the memory could be
left uninitalized and lead to unpredictable results. The testing
compared these invalid results to 0, which seemed to work but is
probably unstable.

This was partially fixed by a previous change that consolidated to
mapping of cell data with a general routine that permuted data. However,
the fix did not extend to point data in the input, and it was not
possible to specify a particular invalid value.

This change specifically updates the probe filter so that invalid values
are set to a user-specified value.


## Avoid raising errors when operating on cells

Cell operations like interpolate and finding parametric coordinates can
fail under certain conditions. The previous behavior was to call
`RaiseError` on the worklet. By design, this would cause the worklet
execution to fail. However, that makes the worklet unstable for a conditin
that might be relatively common in data. For example, you wouldn't want a
large streamline worklet to fail just because one cell was not found
correctly.

To work around this, many of the cell operations in the execution
environment have been changed to return an error code rather than raise an
error in the worklet.

### Error Codes

To support cell operations efficiently returning errors, a new enum named
`vtkm::ErrorCode` is available. This is the current implementation of
`ErrorCode`.

``` cpp
enum class ErrorCode
{
  Success,
  InvalidShapeId,
  InvalidNumberOfPoints,
  WrongShapeIdForTagType,
  InvalidPointId,
  InvalidEdgeId,
  InvalidFaceId,
  SolutionDidNotConverge,
  MatrixFactorizationFailed,
  DegenerateCellDetected,
  MalformedCellDetected,
  OperationOnEmptyCell,
  CellNotFound,

  UnknownError
};
```

A convenience function named `ErrorString` is provided to make it easy to
convert the `ErrorCode` to a descriptive string that can be placed in an
error.

### New Calling Specification

Previously, most execution environment functions took as an argument the
worklet calling the function. This made it possible to call `RaiseError` on
the worklet. The result of the operation was typically returned. For
example, here is how the _old_ version of interpolate was called.

``` cpp
FieldType interpolatedValue =
  vtkm::exec::CellInterpolate(fieldValues, pcoord, shape, worklet);
```

The worklet is now no longer passed to the function. It is no longer needed
because an error is never directly raised. Instead, an `ErrorCode` is
returned from the function. Because the `ErrorCode` is returned, the
computed result of the function is returned by passing in a reference to a
variable. This is usually placed as the last argument (where the worklet
used to be). here is the _new_ version of how interpolate is called.

``` cpp
FieldType interpolatedValue;
vtkm::ErrorCode result =
  vtkm::exec::CellInterpolate(fieldValues, pcoord, shape, interpolatedValue);
```

The success of the operation can be determined by checking that the
returned `ErrorCode` is equal to `vtkm::ErrorCode::Success`.


## Add atomic free functions

Previously, all atomic functions were stored in classes named
`AtomicInterfaceControl` and `AtomicInterfaceExecution`, which required
you to know at compile time which device was using the methods. That in
turn means that anything using an atomic needed to be templated on the
device it is running on.

That can be a big hassle (and is problematic for some code structure).
Instead, these methods are moved to free functions in the `vtkm`
namespace. These functions operate like those in `Math.h`. Using
compiler directives, an appropriate version of the function is compiled
for the current device the compiler is using.

## Flying Edges

Added the flying edges contouring algorithm to VTK-m. This algorithm only
works on structured grids, but operates much faster than the traditional
Marching Cubes algorithm.

The speed of VTK-m's flying edges is comprable to VTK's running on the same
CPUs. VTK-m's implementation also works well on CUDA hardware.

The Flying Edges algorithm was introduced in this paper:

Schroeder, W.; Maynard, R. & Geveci, B.
"Flying edges: A high-performance scalable isocontouring algorithm."
Large Data Analysis and Visualization (LDAV), 2015.
DOI 10.1109/LDAV.2015.7348069


## Filters specify their own field types

Previously, the policy specified which field types the filter should
operate on. The filter could remove some types, but it was not able to
add any types.

This is backward. Instead, the filter should specify what types its
supports and the policy may cull out some of those.

# Build

## Disable asserts for CUDA architecture builds

`assert` is supported on recent CUDA cards, but compiling it appears to be
very slow. By default, the `VTKM_ASSERT` macro has been disabled whenever
compiling for a CUDA device (i.e. when `__CUDA_ARCH__` is defined).

Asserts for CUDA devices can be turned back on by turning the
`VTKm_NO_ASSERT_CUDA` CMake variable off. Turning this CMake variable off
will enable assertions in CUDA kernels unless there is another reason
turning off all asserts (such as a release build).

## Disable asserts for HIP architecture builds

`assert` is supported on recent HIP cards, but compiling it is very slow,
as it triggers the usage of `printf` which. Currently (ROCm 3.7) `printf`
has a severe performance penalty and should be avoided when possible.
By default, the `VTKM_ASSERT` macro has been disabled whenever compiling
for a HIP device via kokkos.

Asserts for HIP devices can be turned back on by turning the
`VTKm_NO_ASSERT_HIP` CMake variable off. Turning this CMake variable off
will enable assertions in HIP kernels unless there is another reason
turning off all asserts (such as a release build).

## Add VTKM_DEPRECATED macro

The `VTKM_DEPRECATED` macro allows us to remove (and usually replace)
features from VTK-m in minor releases while still following the conventions
of semantic versioning. The idea is that when we want to remove or replace
a feature, we first mark the old feature as deprecated. The old feature
will continue to work, but compilers that support it will start to issue a
warning that the use is deprecated and should stop being used. The
deprecated features should remain viable until at least the next major
version. At the next major version, deprecated features from the previous
version may be removed.

### Declaring things deprecated

Classes and methods are marked deprecated using the `VTKM_DEPRECATED`
macro. The first argument of `VTKM_DEPRECATED` should be set to the first
version in which the feature is deprecated. For example, if the last
released version of VTK-m was 1.5, and on the master branch a developer
wants to deprecate a class foo, then the `VTKM_DEPRECATED` release version
should be given as 1.6, which will be the next minor release of VTK-m. The
second argument of `VTKM_DEPRECATED`, which is optional but highly
encouraged, is a short message that should clue developers on how to update
their code to the new changes. For example, it could point to the
replacement class or method for the changed feature.

`VTKM_DEPRECATED` can be used to deprecate a class by adding it between the
`struct` or `class` keyword and the class name.

``` cpp
struct VTKM_DEPRECATED(1.6, "OldClass replaced with NewClass.") OldClass
{
};
```

Aliases can similarly be depreciated, except the `VTKM_DEPRECATED` macro
goes after the name in this case.

``` cpp
using OldAlias VTKM_DEPRECATED(1.6, "Use NewClass instead.") = NewClass;
```

Functions and methods are marked as deprecated by adding `VTKM_DEPRECATED`
as a modifier before the return value and any markup (VTKM_CONT, VTKM_EXEC, or VTKM_EXEC_CONT).

``` cpp
VTKM_DEPRECATED(1.6, "You must now specify a tolerance.") void ImportantMethod(double x)
VTKM_EXEC_CONT
{
  this->ImportantMethod(x, 1e-6);
}
```

`enum`s can be deprecated like classes using similar syntax.

``` cpp
enum struct VTKM_DEPRECATED(1.7, "Use NewEnum instead.") OldEnum
{
  OLD_VALUE
};
```

Individual items in an `enum` can also be marked as deprecated and
intermixed with regular items.

``` cpp
enum struct NewEnum
{
  OLD_VALUE1 VTKM_DEPRECATED(1.7, "Use NEW_VALUE instead."),
  NEW_VALUE,
  OLD_VALUE2 VTKM_DEPRECATED(1.7) = 42
};
```

### Using deprecated items

Using deprecated items should work, but the compiler will give a warning.
That is the point. However, sometimes you need to legitimately use a
deprecated item without a warning. This is usually because you are
implementing another deprecated item or because you have a test for a
deprecated item (that can be easily removed with the deprecated bit). To
support this a pair of macros, `VTKM_DEPRECATED_SUPPRESS_BEGIN` and
`VTKM_DEPRECATED_SUPPRESS_END` are provided. Code that legitimately uses
deprecated items should be wrapped in these macros.

``` cpp
VTKM_DEPRECATED(1.6, "You must now specify both a value and tolerance.")
VTKM_EXEC_CONT
void ImportantMethod()
{
  // It can be the case that to implement a deprecated method you need to
  // use other deprecated features. To do that, just temporarily suppress
  // those warnings.
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  this->ImportantMethod(0.0);
  VTKM_DEPRECATED_SUPPRESS_END
}
```

# Other

## Porting layer for future std features

Currently, VTK-m is using C++11. However, it is often useful to use
features in the `std` namespace that are defined for C++14 or later. We can
provide our own versions (sometimes), but it is preferable to use the
version provided by the compiler if available.

There were already some examples of defining portable versions of C++14 and
C++17 classes in a `vtkmstd` namespace, but these were sprinkled around the
source code.

There is now a top level `vtkmstd` directory and in it are header files
that provide portable versions of these future C++ classes. In each case,
preprocessor macros are used to select which version of the class to use.


## Removed OpenGL Rendering Classes

When the rendering library was first built, OpenGL was used to implement
the components (windows, mappers, annotation, etc.). However, as the native
ray casting became viable, the majority of the work has focused on using
that. Since then, the original OpenGL classes have been largely ignored.

It has for many months been determined that it is not work attempting to
maintain two different versions of the rendering libraries as features are
added and changed. Thus, the OpenGL classes have fallen out of date and did
not actually work.

These classes have finally been officially removed.


## Reorganization of `io` directory

The `vtkm/io` directory has been flattened.
Namely, the files in `vtkm/io/reader` and `vtkm/io/writer` have been moved up into `vtkm/io`,
with the associated changes in namespaces.

In addition, `vtkm/cont/EncodePNG.h` and `vtkm/cont/DecodePNG.h` have been moved to a more natural home in `vtkm/io`.


## Implemented PNG/PPM image Readers/Writers

The original implementation of writing image data was only performed as a 
proxy through the Canvas rendering class. In order to implement true support
for image-based regression testing, this interface needed to be expanded upon
to support reading/writing arbitrary image data and storing it in a `vtkm::DataSet`.
Using the new `vtkm::io::PNGReader` and `vtkm::io::PPMReader` it is possible
to read data from files and Cavases directly and store them as a point field
in a 2D uniform `vtkm::DataSet`

```cpp
auto reader = vtkm::io::PNGReader();
auto imageDataSet = reader.ReadFromFile("read_image.png");
```

Similarly, the new `vtkm::io::PNGWriter` and `vtkm::io::PPMWriter` make it possible
to write out a 2D uniform `vtkm::DataSet` directly to a file.  

```cpp
auto writer = vtkm::io::PNGWriter();
writer.WriteToFile("write_image.png", imageDataSet);
```

If canvas data is to be written out, the reader provides a method for converting
a canvas's data to a `vtkm::DataSet`.

```cpp
auto reader = vtkm::io::PNGReader();
auto dataSet = reader.CreateImageDataSet(canvas);
auto writer = vtkm::io::PNGWriter();
writer.WriteToFile("output.png", dataSet);
```


## Updated Benchmark Framework

The benchmarking framework has been updated to use Google Benchmark.

A benchmark is now a single function, which is passed to a macro:

```
void MyBenchmark(::benchmark::State& state)
{
  MyClass someClass;

  // Optional: Add a descriptive label with additional benchmark details:
  state.SetLabel("Blah blah blah.");

  // Must use a vtkm timer to properly capture eg. CUDA execution times.
  vtkm::cont::Timer timer;
  for (auto _ : state)
  {
    someClass.Reset();

    timer.Start();
    someClass.DoWork();
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  // Optional: Report items and/or bytes processed per iteration in output:
  state.SetItemsProcessed(state.iterations() * someClass.GetNumberOfItems());
  state.SetBytesProcessed(state.iterations() * someClass.GetNumberOfBytes());
}
}
VTKM_BENCHMARK(MyBenchmark);
```

Google benchmark also makes it easy to implement parameter sweep benchmarks:

```
void MyParameterSweep(::benchmark::State& state)
{
  // The current value in the sweep:
  const vtkm::Id currentValue = state.range(0);

  MyClass someClass;
  someClass.SetSomeParameter(currentValue);

  vtkm::cont::Timer timer;
  for (auto _ : state)
  {
    someClass.Reset();

    timer.Start();
    someClass.DoWork();
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VTKM_BENCHMARK_OPTS(MyBenchmark, ->ArgName("Param")->Range(32, 1024 * 1024));
```

will generate and launch several benchmarks, exploring the parameter space of
`SetSomeParameter` between the values of 32 and (1024*1024). The chain of
functions calls in the second argument is applied to an instance of
::benchmark::internal::Benchmark. See Google Benchmark's documentation for
more details.

For more complex benchmark configurations, the VTKM_BENCHMARK_APPLY macro
accepts a function with the signature
`void Func(::benchmark::internal::Benchmark*)` that may be used to generate
more complex configurations.

To instantiate a templated benchmark across a list of types, the
VTKM_BENCHMARK_TEMPLATE* macros take a vtkm::List of types as an additional
parameter. The templated benchmark function will be instantiated and called
for each type in the list:

```
template <typename T>
void MyBenchmark(::benchmark::State& state)
{
  MyClass<T> someClass;

  // Must use a vtkm timer to properly capture eg. CUDA execution times.
  vtkm::cont::Timer timer;
  for (auto _ : state)
  {
    someClass.Reset();

    timer.Start();
    someClass.DoWork();
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
}
VTKM_BENCHMARK_TEMPLATE(MyBenchmark, vtkm::List<vtkm::Float32, vtkm::Vec3f_32>);
```

The benchmarks are executed by calling the `VTKM_EXECUTE_BENCHMARKS(argc, argv)`
macro from `main`. There is also a `VTKM_EXECUTE_BENCHMARKS_PREAMBLE(argc, argv, some_string)`
macro that appends the contents of `some_string` to the Google Benchmark preamble.

If a benchmark is not compatible with some configuration, it may call 
`state.SkipWithError("Error message");` on the `::benchmark::State` object and return. This is
useful, for instance in the filter tests when the input is not compatible with the filter.

When launching a benchmark executable, the following options are supported by Google Benchmark:

- `--benchmark_list_tests`: List all available tests.
- `--benchmark_filter="[regex]"`: Only run benchmark with names that match `[regex]`.
- `--benchmark_filter="-[regex]"`: Only run benchmark with names that DON'T match `[regex]`.
- `--benchmark_min_time=[float]`: Make sure each benchmark repetition gathers `[float]` seconds
  of data.
- `--benchmark_repetitions=[int]`: Run each benchmark `[int]` times and report aggregate statistics 
  (mean, stdev, etc). A "repetition" refers to a single execution of the benchmark function, not
  an "iteration", which is a loop of the `for(auto _:state){...}` section.
- `--benchmark_report_aggregates_only="true|false"`: If true, only the aggregate statistics are
  reported (affects both console and file output). Requires `--benchmark_repetitions` to be useful.
- `--benchmark_display_aggregates_only="true|false"`: If true, only the aggregate statistics are
  printed to the terminal. Any file output will still contain all repetition info.
- `--benchmark_format="console|json|csv"`: Specify terminal output format: human readable 
  (`console`) or `csv`/`json` formats.
- `--benchmark_out_format="console|json|csv"`: Specify file output format: human readable 
  (`console`) or `csv`/`json` formats.
- `--benchmark_out=[filename]`: Specify output file.
- `--benchmark_color="true|false"`: Toggle color output in terminal when using `console` output.
- `--benchmark_counters_tabular="true|false"`: Print counter information (e.g. bytes/sec, items/sec)
  in the table, rather than appending them as a label.

For more information and examples of practical usage, take a look at the existing benchmarks in
vtk-m/benchmarking/.


## Provide scripts to build Gitlab-ci workers locally

To simplify reproducing docker based CI workers locally, VTK-m has python program that handles all the
work automatically for you.

The program is located in `[Utilities/CI/reproduce_ci_env.py ]` and requires python3 and pyyaml. 

To use the program is really easy! The following two commands will create the `build:rhel8` gitlab-ci
worker as a docker image and setup a container just as how gitlab-ci would be before the actual
compilation of VTK-m. Instead of doing the compilation, instead you will be given an interactive shell. 

```
./reproduce_ci_env.py create rhel8
./reproduce_ci_env.py run rhel8
```

To compile VTK-m from the the interactive shell you would do the following:
```
> src]## cd build/
> build]## cmake --build .
```


## Replaced `vtkm::ListTag` with `vtkm::List`

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


## Add `ListTagRemoveIf`

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


## Write uniform and rectilinear grids to legacy VTK files

As a programming convenience, all `vtkm::cont::DataSet` written by
`vtkm::io::VTKDataSetWriter` were written as a structured grid. Although
technically correct, it changed the structure of the data. This meant that
if you wanted to capture data to run elsewhere, it would run as a different
data type. This was particularly frustrating if the data of that structure
was causing problems and you wanted to debug it.

Now, `VTKDataSetWriter` checks the type of the `CoordinateSystem` to
determine whether the data should be written out as `STRUCTURED_POINTS`
(i.e. a uniform grid), `RECTILINEAR_GRID`, or `STRUCTURED_GRID`
(curvilinear).

# References

| Feature                                                                   | Merge Request            |
| --------------------------------------------------------------------------| ------------------------ |
| Add Kokkos backend                                                        | Merge-request: !2164     |
| Extract component arrays from unknown arrays                              | Merge-request: !2354     |
| `ArrayHandleGroupVecVariable` holds now one more offset.                  | Merge-request: !1964     |
| Create `ArrayHandleOffsetsToNumComponents`                                | Merge-request: !2299     |
| Implemented ArrayHandleRandomUniformBits and ArrayHandleRandomUniformReal | Merge-request: !2116     |
| `ArrayRangeCompute` works on any array type without compiling device code | Merge-request: !2409     |
| Algorithms for Control and Execution Environments                         | Merge-request: !1920     |
| Redesign of ArrayHandle to access data using typeless buffers             | Merge-request: !2347     |
| `vtkm::cont::internal::Buffer` now can have ownership transferred         | Merge-request: !2200     |
| Provide scripts to build Gitlab-ci workers locally                        | Merge-request: !2030     |
| Configurable default types                                                | Merge-request: !1997     |
| Result DataSet of coordinate transform has its CoordinateSystem changed   | Merge-request: !2099     |
| Precompiled `ArrayCopy` for `UnknownArrayHandle`                          | Merge-request: !2396     |
| Disable asserts for CUDA architecture builds                              | Merge-request: !2157     |
| Portals may advertise custom iterators                                    | Merge-request: !1929     |
| DataSet now only allows unique field names                                | Merge-request: !2099     |
| ArrayHandleDecorator Allocate and Shrink Support                          | Merge-request: !1933     |
| Deprecate ArrayHandleVirtualCoordinates                                   | Merge-request: !2177     |
| Deprecate `DataSetFieldAdd`                                               | Merge-request: !2106     |
| Deprecate Execute with policy                                             | Merge-request: !2093     |
| Virtual methods in execution environment deprecated                       | Merge-request: !2256     |
| Add VTKM_DEPRECATED macro                                                 | Merge-request: !2266     |
| Filters specify their own field types                                     | Merge-request: !2099     |
| Flying Edges                                                              | Merge-request: !2099     |
| Updated Benchmark Framework                                               | Merge-request: !1936     |
| Disable asserts for HIP architecture builds                               | Merge-request: !2270     |
| Implemented PNG/PPM image Readers/Writers                                 | Merge-request: !1967     |
| Reorganization of `io` directory                                          | Merge-request: !2067     |
| Add `ListTagRemoveIf`                                                     | Merge-request: !1901     |
| Masks and Scatters Supported for 3D Scheduling                            | Merge-request: !1975     |
| Improvements to moving data into ArrayHandle                              | Merge-request: !2184     |
| Avoid raising errors when operating on cells                              | Merge-request: !2099     |
| Order asynchronous `ArrayHandle` access                                   | Merge-request: !2130     |
| Enable setting invalid value in probe filter                              | Merge-request: !2122     |
| `ReadPortal().Get(idx)`                                                   | Merge-request: !2078     |
| Recombine extracted component arrays from unknown arrays                  | Merge-request: !2381     |
| Removed old `ArrayHandle` transfer mechanism                              | Merge-request: !2347     |
| Removed OpenGL Rendering Classes                                          | Merge-request: !2099     |
| Scope ExecObjects with Tokens                                             | Merge-request: !1988     |
| Shorter fancy array handle classnames                                     | Merge-request: !1937     |
| Support `ArrayHandleSOA` as a "default" array                             | Merge-request: !2349     |
| Porting layer for future std features                                     | Merge-request: !1977     |
| Add a vtkm::Tuple class                                                   | Merge-request: !1977     |
| UnknownArrayHandle and UncertainArrayHandle for runtime-determined types  | Merge-request: !2202     |
| Added VecFlat class                                                       | Merge-request: !2354     |
| Remove VTKDataSetWriter::WriteDataSet just_points parameter               | Merge-request: !2185     |
| Move VTK file readers and writers into vtkm_io                            | Merge-request: !2100     |
| Write uniform and rectilinear grids to legacy VTK files                   | Merge-request: !2173     |
| Add atomic free functions                                                 | Merge-request: !2223     |
| Replaced `vtkm::ListTag` with `vtkm::List`                                | Merge-request: !1918     |
