# Extract component arrays from unknown arrays

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

## `ArrayExtractComponent`

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

## Extract components from `UnknownArrayHandle`

The principle use case for `ArrayExtractComponent` is to get an
`ArrayHandle` from an unknown array handle without iterating over _every_
possible type. (Rather, we iterate over a smaller set of types.) To
facilitate this, an `ExtractComponent` method has been added to
`UnknownArrayHandle`.

To use `UnknownArrayHandle::ExtractComponent`, you must give it the
component type. You can check for the correct component type by using the
`IsBaseComponentType` method. The method will then return an
`ArrayHandleStride` for the component type specified.

### Example

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

## `ArrayHandleStride`

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

### Extracting from `ArrayHandleBasic`

When extracting from an `ArrayHandleBasic`, we just need to start at the
proper component and skip the length of the `Vec`.

* _offset_: _component_
* _stride_: `N`

### Extracting from `ArrayHandleSOA`

Since each component is held in a separate array, they are densly packed.
Each component could be represented by `ArrayHandleBasic`, but of course we
use `ArrayHandleStride` to keep the type consistent.

* _offset_: 0
* _stride_: 1

### Extracting from `ArrayHandleCartesianProduct`

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

### Extracting from `ArrayHandleUniformPointCoordinates`

This array cannot be represented directly because it is fully implicit.
However, it can be trivially converted to `ArrayHandleCartesianProduct` in
typically very little memory. (In fact, EAVL always represented uniform
point coordinates by explicitly storing a Cartesian product.) Thus, for
very little overhead the `ArrayHandleStride` can be created.

## Runtime overhead of extracting components

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
