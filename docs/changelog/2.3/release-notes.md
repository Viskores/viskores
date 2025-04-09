VTK-m 2.3.0 Release Notes
=======================

# Table of Contents

1. [Core](#Core)
2. [ArrayHandle](#ArrayHandle)
  - Add ArraySetValues.
3. [Filters and Worklets](#Filters_and_Worklets)
  - Add a form of CellInterpolate that operates on whole cell sets.
  - Fix deprecation warning in WorkletCellNeighborhood.
  - Clip: Improve performance and memory consumption.
  - Contour Polygons with Marching Cubes.
  - Better document the creation of Field and CoordinateSystem.
  - ExternalFaces: Improve performance and memory consumption.
  - Fixed winding of triangles of flying edges on GPUs.
  - Enable extracting external faces from curvilinear data.
4. [Control Environment](#Control_Environment)
  - Load options from environment variables.
  - Added log entry when a cast and call fallback is used.
5. [Execution Environment](#Execution_Environment)
  - Automatically initialize Kokkos.
6. [Build](#Build)
  - Fix clang compile issue with a missing tempolate arg list.
  - Fix include for cub::Swap.
  - Circumvent shadow warnings with thrust swap.
7. [Others](#Others)
  - Simplify CellLocatorBase and PointLocatorBase.
  - Add constexpr to Vec methods.

# Core

# ArrayHandle

## Add ArraySetValues

ArraySetValues.h is a new header file that includes functions to set 1 or many values in an array
similarly to how ArrayGetValues.h gets 1 or many values from an array.

# Filters and Worklets

## Add a form of CellInterpolate that operates on whole cell sets

The initial implementation of `CellInterpolate` takes arguments that are
expected from a topology map worklet. However, sometimes you want to
interplate cells that are queried from locators or otherwise come from a
`WholeCellSet` control signature argument.

A new form of `CellInterpolate` is added to handle this case.

## Fix deprecation warning in WorkletCellNeighborhood

There was a use of a deprecated method buried in a support class of
`WorkletCellNeighborhood`. This fixes that deprecation and also adds a
missing test for `WorkletCellNeighborhood` to prevent such things in the
future.

## Clip: Improve performance and memory consumption

The following set of improvements have been implemented for the Clip algorithm:

1. Input points that are kept are determined by comparing their scalar value with the isovalue, instead of checking the
   output cells' connectivity.
2. Output arrays are written only once, and they are not transformed. Due to that, no auxiliary arrays are needed to
   perform the transformations.
3. A fast path for discarded and kept cells has been added, which are the most common cell cases.
4. ClipTables are now more descriptive, and the non-inverted case tables have been imported from VTK, such that both VTK
   and VTK-m produce the same results.
5. Employ batching of points and cells to use less memory and perform less and faster computations.

The new `Clip` algorithm:

On the CPU:

1. Batch size = min(1000, max(1, numberOfElements / 250000)).
2. Memory-footprint (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 10.22x to 99.67x less memory footprint
    2. For almost half is kept: 2.62x to 4.30x less memory footprint
    3. For almost everything is kept: 2.38x to 3.21x less memory footprint
3. Performance (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 1.63x to 7.79x faster
    2. For almost half is kept: 1.75x to 5.28x faster
    3. For almost everything is kept: 1.71x to 5.35x faster

On the GPU:

1. Batch size = 6.
2. Memory-footprint (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 1.71x to 7.75x less memory footprint
    2. For almost half is kept: 1.11x to 1.36x less memory footprint
    3. For almost everything is kept: 1.09x to 1.31x less memory footprint
3. Performance (the bigger the dataset the greater the benefit):
    1. For almost nothing is kept: 1.54x to 9.67x faster
    2. For almost half is kept: 1.38x to 4.68x faster
    3. For almost everything is kept: 1.21x to 4.46x faster

## Contour Polygons with Marching Cubes

Previously, the Marching Cubes contouring algorithm only had case tables for 3D
polyhedra. This means that if you attempted to contour or slice surfaces or
lines, you would not get any output. However, there are many valid use cases for
contouring this type of data.

This change adds case tables for triangles, quadrilaterals, and lines. It also
adds some special cases for general polygons and poly-lines. These special cases
do not use tables. Rather, there is a special routine that iterates over the
points since these cells types can have any number of points.

Note that to preserve the speed of the operation, contours for cells of a single
dimension type are done at one time. By default, the contour filter will try 3D
cells, then 2D cells, then 1D cells. It is also possible to select a particular
cell dimension or to append results from all cell types together. In this latest
case, the output cells will be of type `CellSetExplicit` instead of
`CellSetSingleType`.

## Better document the creation of Field and CoordinateSystem

The constructors for `vtkm::cont::Field` and `vtkm::cont::CoordinateSystem`
were missing from the built user's guide. The construction of these classes
from names, associations, and arrays are now provided in the documentation.

Also added new versions of `AddField` and `AddCoordinateSystem` to
`vtkm::cont::DataSet` that mimic the constructors. This adds some sytatic
sugar so you can just emplace the field instead of constructing and
passing.

## ExternalFaces: Improve performance and memory consumption

`ExternalFaces` now uses a new algorithm that has the following combination of novel characteristics:

1. employs minimum point id as its face hash function instead of FNV1A of canonicalFaceID, that yields cache-friendly
   memory accesses which leads to enhanced performance.
2. employs an atomic hash counting approach, instead of hash sorting, to perform a reduce-by-key operation on the faces'
   hashes.
3. enhances performance by ensuring that the computation of face properties occurs only once and by avoiding the
   processing of a known internal face more than once.
4. embraces frugality in memory consumption, enabling the processing of larger datasets, especially on GPUs.

When evaluated on Frontier super-computer on 4 large datasets:

The new `ExternalFaces` algorithm:

1. has 4.73x to 5.99x less memory footprint
2. is 4.96x to 7.37x faster on the CPU
3. is 1.54x faster on the GPU, and the original algorithm could not execute on large datasets due to its high memory
   footprint

## Fixed winding of triangles of flying edges on GPUs

The flying edges implementation has an optimization where it will traverse
meshes in the Y direction rather than the X direction on the GPU. It
created mostly correct results, but the triangles' winding was the opposite
from the CPU. This was mostly problematic when normals were generated from
the gradients. In this case, the gradient would point from the "back" of
the face, and that can cause shading problems with some renderers.

This has been fixed to make the windings consistent on the GPU with the CPU
and the gradients.

## Enable extracting external faces from curvilinear data

The external faces filter was not working with curvilinear data. The
implementation for structured cells was relying on axis-aligned point
coordinates, which is not the case for curvilinear grids. The implementation now
only relies on the indices in the 3D grid, so it works on structured data
regardless of the point coordinates. This should also speed up the operation.

# Control Environment

## Load options from environment variables

Some common VTK-m options such as the device and log level could be
specified on the command line but not through environment variables. It is
not always possible to set VTK-m command line options, so environment
variables are added.

Also added documentation to the user's guide about what options are
available and how to set them.

## Added log entry when a cast and call fallback is used

Several places in VTK-m use the `CastAndCallForTypesWithFallback` method in
`UnknownArrayHandle`. The method works well for catching both common and
corner cases. However, there was no way to know if the efficient direct
method or the (supposedly) less likely fallback of copying data to a float
array was used. VTK-m now adds a log event, registered at the "INFO" level,
whenever data is copied to a fallback float array. This helps developers
monitor the eficiency of their code.

# Execution Environment

## Automatically initialize Kokkos

Calling `vtkm::cont::Initialize()` is supposed to be optional. However, Kokkos
needs to have `Kokkos::initialize()` called before using some devices such as
HIP. To make sure that Kokkos is properly initialized, the VTK-m allocation for
the Kokkos device now checks to see if `Kokkos::is_initialized()` is true. If it
is not, then `vtkm::cont::Initialize()` is called.

# Build

## Fix clang compile issue with a missing tempolate arg list

Apparently, starting with LLVM clang version 20, if you use the `template`
keyword to highlight a sub-element, you have to provide a template argument
list. This is true even for a method where the template arguments can be
completely determined by the types of the arguments. Fix this problem by
providing an empty template arg list (so the compiler knows what is
templated but still figures out its own types).

Fixes #830

## Fix include for cub::Swap

A problem we have with the `vtkm::Swap` method is that it can be
ambiguous with the `cub::Swap` method that is part of the CUDA CUB
library. We get around this problem by using the CUB version of the
function when it is available.

However, we were missing an include statement that necessarily provided
`cub::Swap`. This function is now explicitly provided so that we no
longer rely on including it indirectly elsewhere.

## Circumvent shadow warnings with thrust swap

We have run into issues with the `nvcc` compiler giving shadow warnings for
the internals of thrust like this:

```
/usr/local/cuda/bin/../targets/x86_64-linux/include/thrust/detail/internal_functional.h: In constructor 'thrust::detail::unary_negate<Predicate>::unary_negate(const Predicate&)':
/usr/local/cuda/bin/../targets/x86_64-linux/include/thrust/detail/internal_functional.h:45:46: warning: declaration of 'pred' shadows a member of 'thrust::detail::unary_negate<Predicate>' [-Wshadow]
   explicit unary_negate(const Predicate& pred) : pred(pred) {}
                                              ^
/usr/local/cuda/bin/../targets/x86_64-linux/include/thrust/detail/internal_functional.h:42:11: note: shadowed declaration is here
   Predicate pred;
           ^
```

These warnings seem to be caused by the inclusion of `thrust/swap.h`. To
prevent this, this header file is no longer included from `vtkm/Swap.h`.

# Others

## Simplify CellLocatorBase and PointLocatorBase

`CellLocatorBase` and `PointLocatorBase` used to use CRTP. However, this
pattern is unnecessary as the only method in the subclass they call is
`Build`, which does not need templating. The base class does not have to
call the `PrepareForExecution` method, so it can provide its own features
to derived classes more easily.

Also moved `CellLocatorBase` and `PointLocatorBase` out of the `internal`
namespace. Although they provide little benefit other than a base class, it
will make documenting its methods easier.

## Add constexpr to Vec methods

The `constexpr` keyword is helpful to add to functions and macros where
possible. Better than `inline`, it tells the compiler that it can perform
optimizations based on analysis of expressions and literals given in the
code. In particular, this should help code that loops over components have
proper optimizations like loop unrolling when using `Vec` types that have
the number of components fixed.
