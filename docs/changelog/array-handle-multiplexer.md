# Add ArrayHandleMultiplexer

`vtkm::cont::ArrayHandleMultiplexer` is a fancy `ArrayHandle` that can
mimic being any one of a list of other `ArrayHandle`s. When declared, a set
of a list of `ArrayHandle`s is given to `ArrayHandleMultiplexer`. To use
the `ArrayHandleMultiplexer` it is set to an instance of one of these other
`ArrayHandle`s. Thus, once you compile code to use an
`ArrayHandleMultiplexer`, you can at runtime select any of the types it
supports.

The intention is convert the data from a `vtkm::cont::VariantArrayHandle`
to a `vtkm::cont::ArrayHandleMultiplexer` of some known types. The
`ArrayHandleMultiplexer` can be compiled statically (that is, no virtual
methods are needed). Although the compiler must implement all possible
implementations of the multiplexer, two or more `ArrayHandleMultiplexer`s
can be used together without having to compile every possible combination
of all of them.

## Motivation

`ArrayHandle` is a very flexible templated class that allows us to use the
compiler to adapt our code to pretty much any type of memory layout or
on-line processing. Unfortunately, the template approach requires the code
to know the exact type during compile time.

That is a problem when retrieving data from a
`vtkm::cont::VariantArrayHandle`, which is the case, for example, when
getting data from a `vtkm::cont::DataSet`. The actual type of the array
stored in a `vtkm::cont::VariantArrayHandle` is generally not known at
compile time at the code location where the data is pulled.

Our first approach to this problem was to use metatemplate programming to
iterate over all possible types in the `VariantArrayHandle`. Although this
works, it means that if two or more `VariantArrayHandle`s are dispatched in
a function call, the compiler needs to generate all possible combinations
of the two. This causes long compile times and large executable sizes. It
has lead us to limit the number of types we support, which causes problems
with unsupported arrays.

Our second approach to this problem was to create `ArrayHandleVirtual` to
hide the array type behind a virtual method. This works very well, but is
causing us significant problems on accelerators. Although virtual methods
are supported by CUDA, there are numerous problems that can come up with
the compiled code (such as unknown stack depths or virtual methods
extending across libraries). It is also unknown what problems we will
encounter with other accelerator architectures.

`ArrayHandleMultiplexer` is meant to be a compromise between these two
approaches. Although we are still using metatemplate programming tricks to
iterate over multiple implementations, this compiler looping is localized
to the code to lookup values in the array. This, it is a small amount of
code that needs to be created for each version supported by the
`ArrayHandle`. Also, the code paths can be created independently for each
`ArrayHandleMultiplexer`. Thus, you do not get into the problem of a
combinatorial explosion of types that need to be addressed.

Although `ArrayHandleMultiplexer` still has the problem of being unable to
store a type that is not explicitly listed, the localized expression should
allow us to support many types. By default, we are adding lots of
`ArrayHandleCast`s to the list of supported types. The intention of this is
to allow a filter to specify a value type it operates on and then cast
everything to that type. This further allows us to reduce combination of
types that we have to support.

## Use

The `ArrayHandleMultiplexer` templated class takes a variable number of
template parameters. All the template parameters are expected to be types
of `ArrayHandle`s that the `ArrayHandleMultiplexer` can assume.

For example, let's say we have a use case where we need an array of
indices. Normally, the indices are sequential (0, 1, 2,...), but sometimes
we need to define a custom set of indices. When the indices are sequential,
then an `ArrayHandleIndex` is the best representation. Normally if you also
need to support general arrays you would first have to deep copy the
indices into a physical array. However, with an `ArrayHandleMultiplexer`
you can support both.

``` cpp
vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandleIndex,
                                   vtkm::cont::ArrayHandle<vtkm::Id>> indices;
indices = vtkm::cont::ArrayHandleIndex(ARRAY_SIZE);
```

`indices` can now be used like any other `ArrayHandle`, but for now is
behaving like an `ArrayHandleIndex`. That is, it takes (almost) no actual
space. But if you need to use explicit indices, you can set the `indices`
array to an actual array of indices

``` cpp
vtkm::cont::ArrayHandle<vtkm::Id> indicesInMemory;
// Fill indicesInMemory...

indices = indicesInMemory;
```

All the code that uses `indices` will continue to work.

## Variant

To implement `ArrayHandleMultiplexer`, the class `vtkm::internal::Variant`
was introduced. Although this is an internal class that is not exposed
through the array handle, it is worth documenting its addition as it will
be useful to implement other multiplexing type of objects (such as for
cell sets and locators).

`vtkm::internal::Variant` is a simplified version of C++17's `std::variant`
or boost's `variant`. One of the significant differences between VTK-m's
`Variant` and these other versions is that VTK-m's version does not throw
exceptions on error. Instead, behavior becomes undefined. This is
intentional as not all platforms support exceptions and there could be
consequences on just the possibility for those that do.

Like the aforementioned classes that `vtkm::internal::Variant` is based on,
it behaves much like a `union` of a set of types. Those types are listed as
the `Variant`'s template parameters. The `Variant` can be set to any one of
these types either through construction or assignment. You can also use the
`Emplace` method to construct the object in a `Variant`.

``` cpp
vtkm::internal::Variant<int, float, std::string> variant(5);
// variant is now an int.

variant = 5.0f;
// variant is now a float.

variant.Emplace<std::string>("Hello world");
// variant is now an std::string.
```

The `Variant` maintains the index of which type it is holding. It has
several helpful items to manage the type and index of contained objects:

  * `GetIndex()`: A method to retrieve the template parameter index of the
    type currently held. In the previous example, the index starts at 0,
    becomes 1, then becomes 2.
  * `GetIndexOf<T>()`: A static method that returns a `constexpr` of the
    index of a given type. In the previous example,
    `variant.GetIndexOf<float>()` would return 1.
  * `Get<T or I>()`: Given a type, returns the contained object as that
    type. Given a number, returns the contained object as a type of the
    corresponding index. In the previous example, either `variant.Get<1>()`
    or `variant.Get<float>()` would return the `float` value. The behavior
    is undefined if the object is not the requested type.
  * `IsValid()`: A method that can be used to determine whether the
    `Variant` holds an object that can be operated on.
  * `Reset()`: A method to remove any contained object and restore to an
    invalid state.

Finally, `Variant` contains a `CastAndCall` method. This method takes a
functor followed by a list of optional arguments. The contained object is
cast to the appropriate type and the functor is called with the cast object
followed by the provided arguments. If the functor returns a value, that
value is returned by `CastAndCall`.

`CastAndCall` is an important functionality that makes it easy to wrap
multiplexer objects around a `Variant`. For example, here is how you could
implement executing the `Value` method in an implicit function multiplexer.

``` cpp
class ImplicitFunctionMultiplexer
{
  vtkm::internal::Variant<Box, Plane, Sphere> ImplicitFunctionVariant;
  
  // ...
  
  struct ValueFunctor
  {
    template <typename ImplicitFunctionType>
	vtkm::FloatDefault operator()(const ImplicitFunctionType& implicitFunction,
	                              const vtkm::Vec<vtkm::FloatDefault, 3>& point)
	{
	  return implicitFunction.Value(point);
	}
  };
  
  vtkm::FloatDefault Value(const vtkm::Vec<vtkm::FloatDefault, 3>& point) const
  {
    return this->ImplicitFunctionVariant.CastAndCall(ValueFunctor{}, point);
  }

```

