VTK-m 1.8 Release Notes
=======================

# Table of Contents

1. [Core](#Core)

- New `vtkm::List` features
- No longer use brigand.hpp
- Rename field associations
- Favor scoped `enum`s
- UnknownCellSet
- Add implementation of `VecTraits` for `Range` and `Bounds`

2. [ArrayHandle](#ArrayHandle)

- ArrayHandle::Fill
- Make ArrayCopy not depend on a device compiler
- Better fallback for ArrayGetValue

3. [Worklets and Filters](#Worklets-and-Filters)

- Add `CreateResult` to `NewFilter` and absorb field mapping
- NewFilterField
- New Filter Interface Design
- Perlin Noise source

4. [Build](#Build)

- Enable CMAKE_CUDA_ARCHITECTURES
- Enable Unity build -
- Generalized instantiation
- Fix compile when testing is turned off

5. [Other](#Other)

- Redesign of Render Regression Tests


# Core

## New `vtkm::List` features

New features were added to those available in `vtkm/List.h`. These new
features provide new operations on lists.

### Reductions

The new `vtkm::ListReduce` allows a reduction on a list. This template
takes three arguments: a `vtkm::List`, an operation, and an initial value.
The operation is itself a template that has two type arguments.

`vtkm::ListReduce` applies the initial value and the first item of the list
to the operator. The result of that template is then iteratively applied to
the operator with the next item in the list and so on.

``` cpp
// Operation to use
template <typename T1, typename T2>
using Add = std::integral_constant<typename T1::type, T1::value + T2::value>;

using MyList = vtkm::List<std::integral_constant<int, 25>,
                          std::integral_constant<int, 60>,
                          std::integral_constant<int, 87>,
                          std::integral_constant<int, 62>>;

using MySum = vtkm::ListReduce<MyList, Add, std::integral_constant<int, 0>>;
// MySum becomes std::integral_constant<int, 234> (25+60+87+62 = 234)
```

### All and Any

Because they are very common, two reductions that are automatically
supported are `vtkm::ListAll` and `vtkm::ListAny`. These both take a
`vtkm::List` containing either `std::true_type` or `std::false_type` (or
some other "compatible" type that has a constant static `bool` named
`value`). `vtkm::ListAll` will become `std::false_type` if any of the
entries in the list are `std::false_type`. `vtkm::ListAny` becomes
`std::true_type` if any of the entires in the list are `std::true_type`.

``` cpp
using MyList = vtkm::List<std::integral_constant<int, 25>,
                          std::integral_constant<int, 60>,
                          std::integral_constant<int, 87>,
                          std::integral_constant<int, 62>>;

template <typename T>
using IsEven = std::integral_constant<bool, ((T % 2) == 0)>;

// Note that vtkm::ListTransform<MyList, IsEven> becomes
// vtkm::List<std::false_type, std::true_type, std::false_type, std::true_type>

using AllEven = vtkm::ListAll<vtkm::ListTransform<MyList, IsEven>>;
// AllEven becomes std::false_type

using AnyEven = vtkm::ListAny<vtkm::ListTransform<MyList, IsEven>>;
// AnyEven becomes std::true_type
```


## No longer use brigand.hpp

Remove brigand.hpp from VTK-m's source and all references to it. This was
declared in an internal directory, so making this backward-incompatible
changes should be OK.

Brigand is a third-party library to support template meta-programming. Over
the years, we have had to make a few modifications to make sure it compiles
with all compilers supported by VTK-m. Unfortunately, because brigand was
added before our standard third-party library management was set up, these
changes are not managed well. Thus, we cannot easily update with any
changes from the project. Thus, our version is slowly diverging from the
original, and maintaining it is a hassle.

Also, we have been using brigand less and less throughout the years. Now
that we have moved on to C++11 (and now C++14) with variadic templates and
other useful `std` features, the features of brigand have become less
critical. Thus, we have implemented all the features we need from brigand
internally and have moved our code away from using it.


## Rename field associations

The symbols in `vtkm::cont::Field::Association` have been changed from
`ANY`, `WHOLE_MESH`, `POINTS`, and `CELL_SET` to `Any`, `WholeMesh`,
`Points`, and `Cells`, respectively. The reason for this change is twofold:

  * The general standard that VTK-m follows for `enum struct` enumerators
    is to use camel case (with the first character capitalized), not all
    upper case.
  * The use of `CELL_SET` for fields associated with cells is obsolete. A
    `DataSet` used to support having more than one `CellSet`, and so a
    field association on cells was actually bound to a particular
    `CellSet`. However, that is no longer the case. A `DataSet` has exactly
    one `CellSet`, so a cell field no longer has to point to a `CellSet`.
    Thus the enumeration symbol for `Cells` should match the one for
    `Points`.

For backward compatibility, the old enumerations still exist. They are
aliases for the new names, and they are marked as deprecated, so using them
will result in a compiler warning (on some systems).


## Favor scoped `enum`s

Several `enum` declarations were changed from a standard `enum` to a
"scoped" `enum` (i.e. `enum struct`). The advantage of a scoped enum is
that they provide better type safety because they won't be converted
willy-nilly to other types. They also prevent the names they define from
being accessible on the inner scope.

There are some cases where you do want the `enum` to convert to other types
(but still want the scope of the symbols to be contained in the `enum`
type). In this case, we worked around the problem by placing an unscoped
`enum` inside of a standard `struct`.


## UnknownCellSet

The `DynamicCellSet` class has been replaced with `UnknownCellSet`.
Likewise, the `DynamicCellSetBase` class (a templated version of
`DynamicCellSet`) has been replaced with `UncertainCellSet`.

These changes principally follow the changes to the `UnknownArrayHandle`
management class. The `ArrayHandle` version of a polymorphic manager has
gone through several refinements from `DynamicArrayHandle` to
`VariantArrayHandle` to its current form as `UnknownArrayHandle`.
Throughout these improvements for `ArrayHandle`, the equivalent classes for
`CellSet` have lagged behind. The `CellSet` version is decidedly simpler
because `CellSet` itself is polymorphic, but there were definitely
improvements to be had.

The biggest improvement was to remove the templating from the basic unknown
cell set. The old `DynamicArrayHandle` was actually a type alias for
`DynamicArrayHandleBase<VTKM_DEFAULT_CELL_SET_LIST>`. As
`VTKM_DEFAULT_CELL_SET_LIST` tends to be pretty long, `DynamicArrayHandle`
was actually a really long type. In contrast, `UnknownArrayHandle` is its
own untemplated class and will show up in linker symbols as such.


## Add implementation of `VecTraits` for `Range` and `Bounds`

Added specializations of `vtkm::VecTraits` for the simple structures of
`vtkm::Range` and `vtkm::Bounds`. This expands the support for using these
structures in things like `ArrayHandle` and `UnknownArrayHandle`.


# ArrayHandle

## ArrayHandle::Fill

`ArrayHandle` has a new method named `Fill`. As the name would suggest, the
`Fill` method initializes all elements in the array to a specified value.
In addition to being more convenient than calling `Algorithm::Fill` or
`ArrayCopy` with a constant array, the `ArrayHandle::Fill` can be used
without using a device adapter.

Calling `Fill` directly requires the `ArrayHandle` to first be allocated to
the appropriate size. The `ArrayHandle` now also has a new member named
`AllocateAndFill`. As the name would suggest, this method resizes the array
and then fills it with the specified value. Another feature this method has
over calling `Allocate` and `Fill` separately is if you call
`AllocateAndFill` with `vtkm::CopyFlag::On`, it will fill only the extended
portion of the array.

Also added a similar `Fill` and `AllocateAndFill` methods to `BitField` for
similar reasons.


## Make ArrayCopy not depend on a device compiler

Rather than require `ArrayCopy` to create special versions of copy for
all arrays, use a precompiled versions. This should speed up compiles,
reduce the amount of code being generated, and require the device
compiler on fewer source files.

There are some cases where you still need to copy arrays that are not
well supported by the precompiled versions in `ArrayCopy`. (It will
always work, but the fallback is very slow.) In this case, you will want
to switch over to `ArrayCopyDevice`, which has the old behavior.


## Better fallback for ArrayGetValue

To avoid having to use a device compiler every time you wish to use
`ArrayGetValue`, the actual implementation is compiled into the `vtkm_cont`
library. To allow this to work for all the templated versions of
`ArrayHandle`, the implementation uses the extract component features of
`UnknownArrayHandle`. This works for most common arrays, but not all
arrays.

For arrays that cannot be directly represented by an `ArrayHandleStride`,
the fallback is bad. The entire array has to be pulled to the host and then
copied serially to a basic array.

For `ArrayGetValue`, this is just silly. So, for arrays that cannot be
simply represented by `ArrayHandleStride`, make a fallback that just uses
`ReadPortal` to get the data. Often this is not the most efficient method,
but it is better than the current alternative.

# Worklets and Filters

## Add `CreateResult` to `NewFilter` and absorb field mapping

The original version of `Filter` classes had a helper header file named
`CreateResult.h` that had several forms of a `CreateResult` function that
helped correctly create the `DataSet` to be returned from a filter's
`DoExecute`. With the move to the `NewFilter` structure, these functions
did not line up very well with how `DataSet`s should actually be created.

A replacement for these functions have been added as protected helper
methods to `NewFilter` and `NewFilterField`. In addition to moving them
into the filter themselves, the behavior of `CreateResult` has been merged
with the map field to output functionality. The original implementation of
`Filter` did this mapping internally in a different step. The first design
of `NewFilter` required the filter implementer to call a
`MapFieldsOntoOutput` themselves. This new implementation wraps the
functionality of `CreateResult` and `MapFieldsOntoOutput` together so that
the `DataSet` will be created correctly with a single call to
`CreateResult`. This makes it easier to correctly create the output.


## NewFilterField

As part of the New Filter Interface Design, `FilterField`, `FilterDataSet`
and `FilterDataSetWithField` are now refactored into a single
`NewFilterField`. A `NewFilterField` takes an input `DataSet` with some
`Fields`, operates on the input `CellSet` and/or `Field` and generates an
output DataSet, possibly with a new `CellSet` and/or `Field`.

Unlike the old `FilterField`, `NewFilterField` can support arbitrary number
of *active* `Field`s. They are set by the extended `SetActiveField` method
which now also takes an integer index.

`SupportedType` and `Policy` are no longer supported or needed by
`NewFilterField`. Implementations are given full responsibility of extracting
an `ArrayHandle` with proper value and storage type list from an input `Field`
(through a variant of `CastAndCall`). Automatic type conversion from unsupported
value types to `FloatDefault` is also added to UnknownArrayHandle. See
`DotProduct::DoExecute` for an example on how to use the new facility.


## New Filter Interface Design ##

An overhaul of the Filter interface is undergoing. This refactoring effort will
address many problems we faced in the old design. The most important one is to
remove the requirement to compile every single Filter users with a Device Compiler.
This is addressed by removing C++ template (and CRTP) from Filter and is subclasses.
A new non-templated NewFilter class is added with many old templated public interface
removed.

This new design also made Filter implementations thread-safe by default. Filter
implementations are encouraged to take advantage of the new design and removing
shared metatable states from their `DoExecute`, see Doxygen documentation in
NewFilter.h

Filter implementations are also re-organized into submodules, with each submodule
in its own `vtkm/filter` subdirectory. User should update their code to include
the new header files, for example, `vtkm/filter/field_transform/GenerateIds.h`and
link to submodule library file, for example, `libvtkm_filter_field_transform.so`.
To maintain backward compatability, old `vtkm/filter/FooFilter.h` header files
can still be used but will be deprecated in release 2.0.


## Perlin Noise source

A new source, `vtkm::source::PerlinNoise`, has been added. As the name
would imply, this source generates a pseudo-random [Perlin
noise](https://en.wikipedia.org/wiki/Perlin_noise) field.

The field is defined on a 3D grid of specified dimensions. A seed value can
also be specified to enforce consistent results in, for example, test code.
If a seed is not specified, one will be created based on the current system
time.

Perlin noise is useful for testing purposes as it can create non-trivial
geometry at pretty much any scale.

# Build

## Enable CMAKE_CUDA_ARCHITECTURES

When using _CMake_ > 3.18, `CMAKE_CUDA_ARCHITECTURES` can now be used instead of
`VTKm_CUDA_Architecture` to specify the list of architectures desired for the
compilation of _CUDA_ sources. 

Since `CMAKE_CUDA_ARCHITECTURES` is the canonical method of specifying _CUDA_
architectures in _CMake_ and it is more flexible, for instance we can also
specify _CUDA_ virtual architectures, from _CMake_ 3.18 explicitly setting
`VTKm_CUDA_Architecture` will be deprecated whilst still supported.


## Enable Unity build ##

VTK-m now partially supports unity builds in a subset its sources files which
are known to take the longer time/memory to build. Particularly, this enables
you to speedup compilation in VTK-m not memory intensive builds (HIP, CUDA) in a
system with sufficient resources.

We use `BATCH` unity builds type and the number of source files per batch can be
controlled by the canonical _CMake_ variable: `CMAKE_UNITY_BUILD_BATCH_SIZE`.

Unity builds requires _CMake_ >= 3.16, if using a older version, unity build
will be disabled a regular build will be performed.


## Generalized instantiation

Recently, an instantiation method was added to the VTK-m configuration
files to set up a set of source files that compile instances of a template.
This allows the template instances to be compiled exactly once in separate
build files.

However, the implementation made the assumption that the instantiations
were happening for VTK-m filters. Now that the VTK-m filters are being
redesigned, this assumption is broken.

Thus, the instantiation code has been redesigned to be more general. It can
now be applied to code within the new filter structure. It can also be
applied anywhere else in the VTK-m source code.


## Fix compile when testing is turned off

There were some places in VTK-m's code that included test header files even
though they were not tests. As more code goes into libraries, this can
break the build.

Remove VTK-m library dependence on testing code where found. Also added a
CI build that turns off all testing to check for this condition in the
future.

# Other

## Redesign of Render Regression Tests

The helper functions for creating the render regression tests have been
reformulated. The main changes are outlined here.

### Helper functions are no longer templated

The principle change made is that the `RenderAndRegressionTest` has been
changed to no longer require template arguments (which were used to specify
which rendering components to use). However, using templated arguments
requires each rendering test to entirely recompile the rendering code that
it uses. Since the rendering code currently is itself templated, this leads
to a significant amount of re-compilation.

As a side effect of this, the render helper function is now compiled into a
new library, `vtkm_rendering_testing`. Once again, this allows multiple
tests to use rendering without having to recompile the rendering code.

As part of the change, the name of the `RenderAndRegressionTest` function
has been simplified to `RenderTest`.

### Use a more efficient device

It is common for the testing infrastructure to run the same test multiple
times with different devices. Thus, a filter might be run once with the
Kokkos backend and once with the Serial backend. However, even if the
filter is being tested with the serial backend, there is no real reason to
restrict the rendering to a serial process.

Thus, unless otherwise specified, the rendering will use whatever device is
available regardless of what was requested for the test.

### Consolidate options into a struct

Before these changes, there were several options that could be provided to
the render function, and these changes have added several more. The
previous version of the render function specified each of these options as
arguments to the function. However, that quickly became unwieldy as the
number of options grows. Also, it was impossible to send options to the
image comparison (which is called as a subprocess) such as threshold
values.

### Move general testing methods to library

A side effect of these changes is that some more general testing methods
have been moved to the `vtkm_cont_testing` library. Previously, all methods
in the `vtkm::cont::testing::Testing` class were inlined in the header
file. This makes sense for the methods that are templated, but not so much
for methods that are not templated.

Although this change provides minimal improvements with compile times and
object sizes (maybe). But the real benefit is that some of these methods
declare static objects. When declared in inlined functions, a different
object will be created for each translation unit. This can lead to
unexpected behavior when multiple versions of a supposed singleton static
object exist. In particular, this was causing a failure when the static
objects holding testing directories was created by the test translation
unit but was then unavailable to `vtkm_rendering_testing`.

### Expand test_equal_images

The `test_equal_images` function has been expanded to supply the generated
image in a `Canvas` or a `DataSet` in addition to a `View`. Much of the
templating code has been removed from `test_equal_images` and most of the
code has moved into the `vtkm_rendering_testing` library.
