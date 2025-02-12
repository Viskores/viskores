VTK-m 2.2.0 Release Notes
=======================

# Table of Contents

1. [Core](#Core)
  - Enable non-finite values with Intel compiler.
2. [ArrayHandle](#ArrayHandle)
  - Add `GetNumberOfComponentsFlat` method to `ArrayHandle`.
  - Support `Fill` for `ArrayHandleStride`.
  - Added support for component extraction from ArrayHandleConstant better.
  - Store constant AMR arrays with less memory.
  - Improved = operators in VecFromPortal.
  - Deprecate the GetCounts() method in Keys objects.
  - Enable new instances of unknown arrays with dynamic sizes.
3. [Filters and Worklets](#Filters_and_Worklets)
  - Consolidate WarpScalar and WarpVector filter.
  - Adding MergeDataSets filter.
  - Fix bug with ExtractGeometry filter.
  - New Isosurface Uncertainty Visualization Analysis filter.
  - Deprecated `vtkm::filter::FilterField`.
  - Adding SliceMultiple filter.
  - Constructors for mesh info classes updated to conform with other filters.
  - Allow floating-point isovalues for contours of integer fields.
4. [Control Environment](#Control_Environment)
5. [Execution Environment](#Execution_Environment)
  - AtomicArrayExecutionObject: Allow order of atomic operations.
  - Disable Thrust patch that is no longer needed in modern Thrust.
  - Add hints to device adapter scheduler.
6. [Build](#Build)
  - Support backward compatibility in CMake package.
  - Fix issue with union placeholder on Intel compiler.
  - Fix old cuda atomics.
7. [Others](#Others)
  - Avoid floating point exceptions in rendering code.
  - Added VTK-m's user guide into the source code.
  - Fix crash when CUDA device is disabled.

# Core

## Enable non-finite values with Intel compiler

The Intel compiler by default turns on an optimization that assumes that
all floating point values are finite. This breaks any ligitimate uses of
non-finite values including checking values with functions like `isnan`
and `isinf`. Turn off this feature for the intel compiler.

# ArrayHandle

## Add `GetNumberOfComponentsFlat` method to `ArrayHandle`

Getting the number of components (or the number of flattened components)
from an `ArrayHandle` is usually trivial. However, if the `ArrayHandle` is
special in that the number of components is specified at runtime, then it
becomes much more difficult to determine.

Getting the number of components is most important when extracting
component arrays (or reconstructions using component arrays) with
`UnknownArrayHandle`. Previously, `UnknownArrayHandle` used a hack to get
the number of components, which mostly worked but broke down when wrapping
a runtime array inside another array such as `ArrayHandleView`.

To prevent this issue, the ability to get the number of components has been
added to `ArrayHandle` proper. All `Storage` objects for `ArrayHandle`s now
need a method named `GetNumberOfComponentsFlat`. The implementation of this
method is usually trivial. The `ArrayHandle` template now also provides a
`GetNumberOfComponentsFlat` method that gets this information from the
`Storage`. This provides an easy access point for the `UnknownArrayHandle`
to pull this information.

## Support `Fill` for `ArrayHandleStride`

Previously, if you called `Fill` on an `ArrayHandleStride`, you would get
an exception that said the feature was not supported. It turns out that
filling values is very useful in situations where, for example, you need to
initialize an array when processing an unknown type (and thus dealing with
extracted components).

This implementation of `Fill` first attempts to call `Fill` on the
contained array. This only works if the stride is set to 1. If this does
not work, then the code leverages the precompiled `ArrayCopy`. It does this
by first creating a new `ArrayHandleStride` containing the fill value and a
modulo of 1 so that value is constantly repeated. It then reconstructs an
`ArrayHandleStride` for itself with a modified size and offset to match the
start and end indices.

Referencing the `ArrayCopy` was tricky because it kept creating circular
dependencies among `ArrayHandleStride`, `ArrayExtractComponent`, and
`UnknownArrayHandle`. These dependencies were broken by having
`ArrayHandleStride` directly use the internal `ArrayCopyUnknown` function
and to use a forward declaration of `UnknownArrayHandle` rather than
including its header.

## Added support for component extraction from ArrayHandleConstant better

Previously, `ArrayHandleConstant` did not really support component
extraction. Instead, it let a fallback operation create a full array on
the CPU.

Component extraction is now quite efficient for `ArrayHandleConstant`. It
creates a basic `ArrayHandle` with one entry and sets a modulo on the
`ArrayHandleStride` to access that value for all indices.

## Store constant AMR arrays with less memory

The `AmrArrays` filter generates some cell fields that specify information
about the hierarchy, which are constant across all cells in a partition.
These were previously stored as an array with the same value throughout.
Now, the field is stored as an `ArrayHandleConstant`, which does not
require any real storage. Recent changes to VTK-m allow code to extract the
array as a component efficiently without knowing the storage type.

## Improved = operators in VecFromPortal

Previously, `VecFromPortal` could only be set to a standard `Vec`.
However, because this is a `Vec`-like object with a runtime-size, it is
hard to do general arithmetic on it. It is easier to do in place so
there is some place to put the result. To make it easier to operate on
this as the result of other `Vec`-likes, extend the operators like `+=`,
`*=`, etc to support this.

## Deprecate the GetCounts() method in Keys objects

The `vtkm::worklet::Keys` object held a `SortedValuesMap` array, an
`Offsets` array, a `Counts` array, and (optionally) a `UniqueKeys` array.
Of these, the `Counts` array is redundant because the counts are trivially
computed by subtracting adjacent entries in the offsets array. This pattern
shows up a lot in VTK-m, and most places we have moved to removing the
counts and just using the offsets.

This change removes the `Count` array from the `Keys` object. Where the
count is needed internally, adjacent offsets are subtracted. The deprecated
`GetCounts` method is implemented by copying values into a new array.

## Enable new instances of unknown arrays with dynamic sizes

`UnknownArrayHandle` allows you to create a new instance of a compatible
array so that when receiving an array of unknown type, a place to put the
output can be created. However, these methods only worked if the number of
components in each value could be determined statically at compile time.

However, there are some special `ArrayHandle`s that can define the number
of components at runtime. In this case, the `ArrayHandle` would throw an
exception if `NewInstanceBasic` or `NewInstanceFloatBasic` was called.

Although rare, this condition could happen when, for example, an array was
extracted from an `UnknownArrayHandle` with `ExtractArrayFromComponents` or
with `CastAndCallWithExtractedArray` and then the resulting array was
passed to a function with arrays passed with `UnknownArrayHandle` such as
`ArrayCopy`.


# Filters and Worklets

## Consolidate WarpScalar and WarpVector filter

In reflection, the `WarpScalar` filter is surprisingly a superset of the
`WarpVector` features. `WarpScalar` has the ability to displace in the
directions of the mesh normals. In VTK, there is a distinction of normals
to vectors, but in VTK-m it is a matter of selecting the correct one. As
such, it makes little sense to have two separate implementations for the
same operation. The filters have been combined and the interface names have
been generalized for general warping (e.g., "normal" or "vector" becomes
"direction").

In addition to consolidating the implementation, the `Warp` filter
implementation has been updated to use the modern features of VTK-m's
filter base classes. In particular, when the `Warp` filters were originally
implemented, the filter base classes did not support more than one active
scalar field, so filters like `Warp` had to manage multiple fields
themselves. The `FilterField` base class now allows specifying multiple,
indexed active fields, and the updated implementation uses this to manage
the input vectors and scalars.

The `Warp` filters have also been updated to directly support constant
vectors and scalars, which is common for `WarpScalar` and `WarpVector`,
respectively. Previously, to implement a constant field, you had to add a
field containing an `ArrayHandleConstant`. This is still supported, but an
easier method of just selecting constant vectors or scalars makes this
easier.

Internally, the implementation now uses tricks with extracting array
components to support many different array types (including
`ArrayHandleConstant`. This allows it to simultaneously interact with
coordinates, directions, and scalars without creating too many template
instances.

## Adding MergeDataSets filter

The MergeDataSets filter can accept partitioned data sets and output a merged
data set. It assumes that all partitions have the same coordinate systems. If a
field is missing in a specific partition, the user-specified invalid value will
be adopted and filled into the corresponding places of the merged field array.

## Fix bug with ExtractGeometry filter

The `ExtractGeometry` filter was outputing datasets containing
`CellSetPermutation` as the representation for the cells. Although this is
technically correct and a very fast implementation, it is essentially
useless. The problem is that any downstream processing will have to know
that the data has a `CellSetPermutation`. None do (because the permutation
can be on any other cell set type, which creates an explosion of possible
cell types).

Like was done with `Threshold` a while ago, this problem is fixed by deep
copying the result into a `CellSetExplicit`. This behavior is consistent
with VTK.

## New Isosurface Uncertainty Visualization Analysis filter

This new filter, designed for uncertainty analysis of the marching cubes
algorithm for isosurface visualization, computes three uncertainty metrics,
namely, level-crossing probability, topology case count, and entropy-based
uncertainty. This filter requires a 3D cell-structured dataset with ensemble
minimum and ensemble maximum values denoting uncertain data range at each grid
vertex.

This uncertainty analysis filter analyzes the uncertainty in the marching cubes
topology cases. The output dataset consists of 3D point fields corresponding to
the level-crossing probability, topology case count, and entropy-based
uncertainty.

This VTK-m implementation is based on the algorithm presented in the paper
"FunMC2: A Filter for Uncertainty Visualization of Marching Cubes on Multi-Core
Devices" by Wang, Z., Athawale, T. M., Moreland, K., Chen, J., Johnson, C. R.,
& Pugmire, D.

## Deprecated `vtkm::filter::FilterField`

The original design of the filter base class required several specialized
base classes to control what information was pulled from the input
`DataSet` and provided to the derived class. Since the filter base class was
redesigned, the derived classes all get a `DataSet` and pull their own
information from it. Thus, most specialized filter base classes became
unnecessary and removed.

The one substantial exception was the `FilterField`. This filter base class
managed input and output arrays. This was kept separate from the base
`Filter` because not all filters need the ability to select this
information.

That said, this separation has not been particularly helpful. There are
several other features of `Filter` that does not apply to all subclasses.
Furthermore, there are several derived filters that are using `FilterField`
merely to pick a single part, like selecting a coordinate system, and
ignoring the rest of the abilities.

Thus, it makes more sense to deprecate `FilterField` and have these classes
inherit directly from `Filter`.

## Adding SliceMultiple filter

The SliceMultiple filter can accept multiple implicit functions and output a
merged dataset. This filter is added to the filter/contour. The code of this
filter is adapted from vtk-h. The mechanism that merges multiple datasets in
this filter is supposed to support more general datasets and work a separate
filter in the future.

## Constructors for mesh info classes updated to conform with other filters

The `CellMeasures` and `MeshQuality` filters had constructors that took the
metric that the filter should generate. However, this is different than the
iterface of the rest of the filters. To make the interface more consistent,
these filters now have a default (no argument) constructor, and the metric
to compute is selected via a method. This makes it more clear what is being
done.

In addition, the documentation for these two classes is updated.

## Allow floating-point isovalues for contours of integer fields

The flying edges version of the contouring filter converted the isovalues
provided into the same type as the field. This is fine for a floating point
field, but for an integer field the isovalue was truncated to the nearest
integer.

This is problematic because it is common to provide a fractional isovalue
(usually N + 0.5) for integer fields to avoid degenerate cases of the
contour intersecting vertices. It also means the behavior changes between
an integer type that is directly supported (like a `signed char`) or an
integer type that is not directly supported and converted to a floating
point field (like potentially a `char`).

This change updates the worklets to allow the isovalue to have a different
type than the field and to always use a floating point type for the
isovalue.


# Control Environment


# Execution Environment

## AtomicArrayExecutionObject: Allow order of atomic operations

AtomicArrayExecutionObject now allows order of atomic operations for Get/Set/Add/CompareExchange.

## Disable Thrust patch that is no longer needed in modern Thrust

There is a Thrust patch that works around an issue in Thrust 1.9.4
(https://github.com/NVIDIA/thrust/issues/972). The underlying issue
should be fixed in recent versions. In recent versions of CUDA, the patch
breaks (https://gitlab.kitware.com/vtk/vtk-m/-/issues/818).

This change fixes the problem by disabling the patch where it is not
needed.

## Add hints to device adapter scheduler

The `DeviceAdapter` provides an abstract interface to the accelerator
devices worklets and other algorithms run on. As such, the programmer has
less control about how the device launches each worklet. Each device
adapter has its own configuration parameters and other ways to attempt to
optimize how things are run, but these are always a universal set of
options that are applied to everything run on the device. There is no way
to specify launch parameters for a particular worklet.

To provide this information, VTK-m now supports `Hint`s to the device
adapter. The `DeviceAdapterAlgorithm::Schedule` method takes a templated
argument that is of the type `HintList`. This object contains a template
list of `Hint` types that provide suggestions on how to launch the parallel
execution. The device adapter will pick out hints that pertain to it and
adjust its launching accordingly.

These are called hints rather than, say, directives, because they don't
force the device adapter to do anything. The device adapter is free to
ignore any (and all) hints. The point is that the device adapter can take
into account the information to try to optimize for itself.

A provided hint can be tied to specific device adapters. In this way, an
worklet can further optimize itself. If multiple hints match a device
adapter, the last one in the list will be selected.

The `Worklet` base now has an internal type named `Hints` that points to a
`HintList` that is applied when the worklet is scheduled. Derived worklet
classes can provide hints by simply defining their own `Hints` type.

This feature is experimental and consequently hidden in an `internal`
namespace.

# Build

## Support backward compatibility in CMake package

VTK-m development is in a mode where backward compatibility should be
maintained between minor versions of the software. (You may get deprecation
warnings, but things should still work.) To match this behavior, the
generated CMake package now supports finding versions with the same major
release and the same or newer minor release. For example, if an external
program does this

``` cmake
find_package(VTKm 2.1 REQUIRED)
```

then CMake will link to 2.1 (of course) as well as newer minor releases
(e.g., 2.2, 2.3, etc.). It will not, however, match older versions (e.g.,
2.0, 1.9), nor will it match any version after the next major release
(e.g., 3.0).

## Fix issue with union placeholder on Intel compiler

We have run into an issue with some Intel compilers where if a `union`
contains a `struct` that has some padding for byte alignment, the value
copy might skip over that padding even when the `union` contains a different
type where those bytes are valid. This breaks the value copy of our
`Variant` class.

This is not a unique problem. We have seen the same thing in other
compilers and already have a workaround for when this happens. The
workaround creates a special struct that has no padding placed at the front
of the `union`. The Intel compiler adds a fun twist in that this
placeholder structure only works if the alignment is at least as high as
the struct that follows it.

To get around this problem, make the alignment of the placeholder `struct`
at large as possible for the size of the `union`.

## Fix old cuda atomics

There are some overloads for atomic adds of floating point numbers for
older versions of cuda that do not include them directly. These were
misnamed and thus not properly overloading the generic implementation.
This caused compile errors with older versions of cuda.

## Fix crash when CUDA device is disabled

There was an issue where if VTK-m was compiled with CUDA support but then
run on a computer where no CUDA device was available, an inappropriate
exception was thrown (instead of just disabling the device). The
initialization code should now properly check for the existance of a CUDA
device.

# Others

## Avoid floating point exceptions in rendering code

There were some places in the rendering code where floating point
exceptions (FPE) could happen under certain circumstances. Often we do not
care about invalid floating point operation in rendering as they often
occur in degenerate cases that don't contribute anyway. However,
simulations that might include VTK-m might turn on FPE to check their own
operations. In such cases, we don't want errant rendering arithmetic
causing an exception and bringing down the whole code. Thus, we turn on FPE
in some of our test platforms and avoid such operations in general.

## Added VTK-m's user guide into the source code

The VTK-m User's Guide is being transitioned into the VTK-m source code.
The implementation of the guide is being converted from LaTeX to
ReStructuredText text to be built by Sphinx. There are several goals of
this change.

1. Integrate the documentation into the source code better to better
   keep the code up to date.
2. Move the documentation over to Sphinx so that it can be posted online
   and be more easily linked.
3. Incoporate Doxygen into the guide to keep the documentation
   consistent.
4. Build the user guide examples as part of the VTK-m CI to catch
   compatibility changes quickly.
