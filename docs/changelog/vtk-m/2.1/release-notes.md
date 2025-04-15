# VTK-m 2.1 Release Notes

# Table of Contents

1. [Core](#Core)
  - Fixed issue with trivial variant copies.
  - Simplified serialization of DataSet objects.
  - Updated the interface and documentation of GhostCellRemove.
  - VTKDataSetReader handles any Vec size.

2. [ArrayHandle](#ArrayHandle)
  - Fix new instances of ArrayHandleRuntimeVec in UnknownArrayHandle.
  - Added `ArrayHandleRuntimeVec` to specify vector sizes at runtime.
  - Fix an issue with copying array from a disabled device.
  - New features for computing array ranges.
  - Added ability to resize strided arrays from ArrayExtractComponent.
  - Support using arrays with dynamic Vec-likes as output arrays.
  - Fast paths for `ArrayRangeCompute` fixed.
  - Added support for getting vec sizes of unknown arrays when runtime selected.

3. [Filters and Worklets](#Filters_and_Worklets)
  - Fixed error in checking paired faces in ExternalFaces filter.
  - Split flying edges and marching cells into separate filters.
  - Make flow filters modular and extensible using traits.
  - Update filters' field map and execute to work on any field type.
  - Tetrahedralize and Triangulate filters now check if the input is already tetrahedral/triangular.
  - Added support for `CastAndCallVariableVecField` in `FilterField`.
  - New Statistics filter.
  - Added a `ConvertToPointCloud` filter.
  - New Composite Vector filter.
  - ComputeMoments filter now operates on any scalar field.
  - Continuous Scatterplot filter.
  - Support any cell type in MIR filter.
  - Get the 3D index from a BoundaryState in WorkletPointNeighborhood.
  - Clip now doesn't copy unused points from the input to the output.
  - Implement Flying Edges for structured cellsets with rectilinear and curvilinear coordinates.
  - Added a `HistSampling` filter.
  - New Shrink filter.
  - Fix interpolation of cell fields with flying edges.
  - Fix degenerate cell removal.

4. [Control Environment](#Control_Environment)
  - Added a reader for VisIt files.
  - Fixed operator for IteratorFromArrayPortal.
  - Implemented `VecTraits` class for all types.

5. [Build](#Build)
  - Require Kokkos 3.7.
  - Sped up compilation of ArrayRangeCompute.cxx.
  - Kokkos atomic functions switched to use desul library.
  - Added Makefile contract tests.

6. [Others](#Others)
  - Clarified license of test data.

# Core

## Fixed issue with trivial variant copies

A rare error occurred with trivial copies of variants. The problem is likely
a compiler bug, and has so far only been observed when passing the variant
to a CUDA kernel when compiling with GCC 5.

The problem was caused by structures with padding. `struct` objects in
C/C++ are frequently padded with unused memory to align all entries
properly. For example, consider the following simple `struct`.

```cpp
struct FooHasPadding
{
  vtkm::Int32 A;
  // Padding here.
  vtkm::Int64 C;
};
```

Because the `C` member is a 64-bit integer, it needs to be aligned on
8-byte (i.e., 64-bit) address locations. For this to work, the C++ compiler
adds 4 bytes of padding between `A` and `C` so that an array of
`FooHasPadding`s will have the `C` member always on an 8-byte boundary.

Now consider a second `struct` that is similar to the first but has a valid
member where the padding would be.

```cpp
struct BarNoPadding
{
  vtkm::Int32 A;
  vtkm::Int32 B;
  vtkm::Int64 C;
};
```

This structure does not need padding because the `A` and `B` members
combine to fill the 8 bytes that `C` needs for the alignment. Both
`FooHasPadding` and `BarNoPadding` fill 16 bytes of memory. The `A` and `C`
members are at the same offsets, respectively, for the two structures. The
`B` member happens to reside just where the padding is for `FooHasPadding`.

Now, let's say we create a `vtkm::exec::Variant<FooHasPadding, BarNoPadding>`.
Internally, the `Variant` class holds a union that looks roughly like the
following.

```cpp
union VariantUnion
{
  FooHasPadding V0;
  BarNoPadding V1;
};
```

This is a perfectly valid use of a `union`. We just need to keep track of
which type of object is in it (which the `Variant` object does for you).

The problem appeared to occur when `VariantUnion` contained a
`BarNoPadding` and was passed from the host to the device via an argument
to a global function. The compiler must notice that the first type
(`FooHasPadding`) is the "biggest" and uses that for trivial copies (which
just copy bytes like `memcpy`). Since it's using `FooHasPadding` as its
prototype for the byte copy, and accidentally skips over padded regions that
are valid when the `union` contains a `BarNoPadding`. This appears to be a
compiler bug. (At least, I cannot find a reason why this is encroaching
undefined behavior.)

The solution adds a new, unused type to the internal `union` for `Variant`
that is an object as large as the largest entry in the union and contains
no padding.

## Simplified serialization of DataSet objects

`vtkm::cont::DataSet` is a dynamic object that can hold cell sets and
fields of many different types, none of which are known until runtime. This
causes a problem with serialization, which has to know what type to compile
the serialization for, particularly when unserializing the type at the
receiving end. The original implementation "solved" the problem by creating
a secondary wrapper object that was templated on types of field arrays and
cell sets that might be serialized. This is not a great solution as it
punts the problem to algorithm developers.

This problem has been completely solved for fields, as it is possible to
serialize most types of arrays without knowing their type now. You still
need to iterate over every possible `CellSet` type, but there are not that
many `CellSet`s that are practically encountered. Thus, there is now a
direct implementation of `Serialization` for `DataSet` that covers all the
data types you are likely to encounter.

The old `SerializableDataSet` has been deprecated. In the unlikely event an
algorithm needs to transfer a non-standard type of `CellSet` (such as a
permuted cell set), it can use the replacement `DataSetWithCellSetTypes`,
which just specifies the cell set types.

## Updated the interface and documentation of GhostCellRemove

The `GhostCellRemove` filter had some methods inconsistent with the naming
convention elsewhere in VTK-m. The class itself was also in need of some
updated documentation. Both of these issues have been fixed.

Additionally, there were some conditions that could lead to unexpected
behavior. For example, if the filter was asked to remove only ghost cells
and a cell was both a ghost cell and blank, it would not be removed. This
has been updated to be more consistent with expectations.

## VTKDataSetReader handles any Vec size.

The legacy VTK file reader previously only supported a specific set of Vec
lengths (i.e., 1, 2, 3, 4, 6, and 9). This is because a basic array handle
has to have the vec length compiled in. However, the new
`ArrayHandleRuntimeVec` feature is capable of reading in any vec-length and
can be leveraged to read in arbitrarily sized vectors in field arrays.

# ArrayHandle

## Fix new instances of ArrayHandleRuntimeVec in UnknownArrayHandle

`UnknownArrayHandle` is supposed to treat `ArrayHandleRuntimeVec` the same
as `ArrayHandleBasic`. However, the `NewInstance` methods were failing
because they need custom handling of the vec size. Special cases in the
`UnknownArrayHandle::NewInstance*()` methods have been added to fix this
problem.

## Added `ArrayHandleRuntimeVec` to specify vector sizes at runtime.

The new `ArrayHandleRuntimeVec` is a fancy `ArrayHandle` allows you to
specify a basic array of `Vec`s where the number of components of the `Vec`
are not known until runtime. (It can also optionally specify scalars.) The
behavior is much like that of `ArrayHandleGroupVecVariable` except that its
representation is much more constrained. This constrained representation
allows it to be automatically converted to an `ArrayHandleBasic` with the
proper `Vec` value type. This allows one part of code (such as a file
reader) to create an array with any `Vec` size, and then that array can be
fed to an algorithm that expects an `ArrayHandleBasic` of a certain value
type.

The `UnknownArrayHandle` has also been updated to allow
`ArrayHandleRuntimeVec` to work interchangeably with basic `ArrayHandle`.
If an `ArrayHandleRuntimeVec` is put into an `UnknownArrayHandle`, it can
be later retrieved as an `ArrayHandleBasic` as long as the base component
type matches and it has the correct amount of components. This means that
an array can be created as an `ArrayHandleRuntimeVec` and be used with any
filters or most other features designed to operate on basic `ArrayHandle`s.
Likewise, an array added as a basic `ArrayHandle` can be retrieved in an
`ArrayHandleRuntimeVec`. This makes it easier to pull arrays from VTK-m and
place them in external structures (such as `vtkDataArray`).

## Fix an issue with copying array from a disabled device

The internal array copy has an optimization to use the device the array
exists on to do the copy. However, if that device is disabled the copy
would fail. This problem has been fixed.

## New features for computing array ranges

ArrayRangeCompute has been update to support more features that are present
in VTK and ParaView.

New overloads for `ArrayRangeCompute` have been added:

1. Takes a boolean parameter, `computeFiniteRange`, that specifies
   whether to compute only the finite range by ignoring any non-finite values (+/-inf)
   in the array.

1. Takes a `maskArray` parameter of type `vtkm::cont::ArrayHandle<vtkm::UInt8>`.
   The mask array must contain the same number of elements as the input array.
   A value in the input array is treated as masked off if the
   corresponding value in the mask array is non-zero. Masked off values are ignored
   in the range computation.

A new function `ArrayRangeComputeMagnitude` has been added. If the input array
has multiple components, this function computes the range of the magnitude of
the values of the array. Nested Vecs are treated as flat. A single `Range` object
is returned containing the result. `ArrayRangeComputMagnitude` also has similar
overloads as `ArrayRangeCompute`.

## Added ability to resize strided arrays from ArrayExtractComponent

Previously, it was not possible to resize an `ArrayHandleStride` because
the operation is a bit ambiguous. The actual array is likely to be padded
by some amount, and there could be an unknown amount of space skipped at
the beginning.

However, there is a good reason to want to resize `ArrayHandleStride`. This
is the array used to implement the `ArrayExtractComponent` feature, and
this in turn is used when extracting arrays from an `UnknownArrayHandle`
whether independent or as an `ArrayHandleRecombineVec`.

The problem really happens when you create an array of an unknown type in
an `UnknownArrayHandle` (such as with `NewInstance`) and then use that as
an output to a worklet. Sure, you could use `ArrayHandle::Allocate` to
resize before getting the array, but that is awkward for programers.
Instead, allow the extracted arrays to be resized as normal output arrays
would be.

## Support using arrays with dynamic Vec-likes as output arrays

When you use an `ArrayHandle` as an output array in a worklet (for example,
as a `FieldOut`), the fetch operation does not read values from the array
during the `Load`. Instead, it just constructs a new object. This makes
sense as an output array is expected to have garbage in it anyway.

This is a problem for some special arrays that contain `Vec`-like objects
that are sized dynamically. For example, if you use an
`ArrayHandleGroupVecVariable`, each entry is a dynamically sized `Vec`. The
array is referenced by creating a special version of `Vec` that holds a
reference to the array portal and an index. Components are retrieved and
set by accessing the memory in the array portal. This allows us to have a
dynamically sized `Vec` in the execution environment without having to
allocate within the worklet.

The problem comes when we want to use one of these arrays with `Vec`-like
objects for an output. The typical fetch fails because you cannot construct
one of these `Vec`-like objects without an array portal to bind it to. In
these cases, we need the fetch to create the `Vec`-like object by reading
it from the array. Even though the data will be garbage, you get the
necessary buffer into the array (and nothing more).

Previously, the problem was fixed by creating partial specializations of
the `Fetch` for these `ArrayHandle`s. This worked OK as long as you were
using the array directly. However, the approach failed if the `ArrayHandle`
was wrapped in another `ArrayHandle` (for example, if an `ArrayHandleView`
was applied to an `ArrayHandleGroupVecVariable`).

To get around this problem and simplify things, the basic `Fetch` for
direct output arrays is changed to handle all cases where the values in the
`ArrayHandle` cannot be directly constructed. A compile-time check of the
array's value type is checked with `std::is_default_constructible`. If it
can be constructed, then the array is not accessed. If it cannot be
constructed, then it grabs a value out of the array.

## Fast paths for `ArrayRangeCompute` fixed

The precompiled `ArrayRangeCompute` function was not following proper fast
paths for special arrays. For example, when computing the range of an
`ArrayHandleUniformPointCoordinates`, the ranges should be taken from the
origin and spacing of the special array. However, the precompiled version
was calling the generic range computation, which was doing an unnecessary
reduction over the entire array. These fast paths have been fixed.

These mistakes in the code were caused by quirks in how templated method
overloading works. To prevent this mistake from happening again in the
precompiled `ArrayRangeCompute` function and elsewhere, all templated forms
of `ArrayRangeCompute` have been deprecated. Most will call
`ArrayRangeCompute` with no issues. For those that need the templated
version, `ArrayRangeComputeTemplate` replaces the old templated
`ArrayRangeCompute`. There is exactly one templated declaration of
`ArrayRangeComputeTemplate` that uses a class, `ArrayRangeComputeImpl`,
with partial specialization to ensure the correct form is used.

## Added support for getting vec sizes of unknown arrays when runtime selected

The `GetNumberOfComponents` and `GetNumberOfComponentsFlat` methods in
`UnknownArrayHandle` have been updated to correctly report the number of
components in special `ArrayHandle`s where the `Vec` sizes of the values
are not selected until runtime.

Previously, these methods always reported 0 because the value type could
not report the size of the `Vec`. The lookup has been modified to query the
`ArrayHandle`'s `Storage` for the number of components where supported.
Note that this only works on `Storage` that provides a method to get the
runtime `Vec` size. If that is not provided, as will be the case if the
number of components can vary from one value to the next, it will still
report 0.

This feature is implemented by looking for a method named
`GetNumberOfComponents` is the `Storage` class for the `ArrayHandle`. If
this method is found, it is used to query the size at runtime.

# Filters and Worklets

## Fixed error in checking paired faces in ExternalFaces filter

The `ExternalFaces` filter uses hash codes to find duplicate (i.e.
internal) faces. The issue with hash codes is that you have to deal with
unique entries that have identical hashes. The worklet to count how many
unique, unmatched faces were associated with each hash code was correct.
However, the code to then grab the ith unique face in a hash was wrong.
This has been fixed.

## Split flying edges and marching cells into separate filters

The contour filter contains 2 separate implementations, Marching Cells and Flying Edges, the latter only available if the input has a `CellSetStructured<3>` and `ArrayHandleUniformPointCoordinates` for point coordinates. The compilation of this filter was lenghty and resource-heavy, because both algorithms were part of the same translation unit.

Now, this filter is separated into two new filters, `ContourFlyingEdges` and `ContourMarchingCells`, compiling more efficiently into two translation units. The `Contour` API is left unchanged. All 3 filters `Contour`, `ContourFlyingEdges` and `ContourMarchingCells` rely on a new abstract class `AbstractContour` to provide configuration and common utility functions.

Although `Contour` is still the preferred option for most cases because it selects the best implementation according to the input, `ContourMarchingCells` is usable on any kind of 3D Dataset. For now, `ContourFlyingEdges` operates only on structured uniform datasets.

Deprecate functions `GetComputeFastNormalsForStructured`, `SetComputeFastNormalsForStructured`, `GetComputeFastNormalsForUnstructured` and `GetComputeFastNormalsForUnstructured`, to use the more general `GetComputeFastNormals` and `SetComputeFastNormals` instead.

By default, for the `Contour` filter, `GenerateNormals` is now `true`, and `ComputeFastNormals` is `false`.

The marching cubes version of contour still has several possible compile paths, so it can still take a bit to compile. To help manage the compile time further, the contour filter compilation is broken up even further using the instantiation build capabilities.

## Make flow filters modular and extensible using traits

Many flow filters have common underpinnings in term of the components they use.
E.g., the choice and handling for solvers, analysis, termination, vector field, etc.
However, having these components baked hard in the infrastructure makes extensibility chanllenging,
which leads to developers implementing bespoke solutions.
This change establishes an infrastructure for easy specification and development of flow filter.

To that end, two new abstractions are introduced along with thier basic implementations : `Analysis` and `Termination`

- `Analysis` defines how each step of the particle needs to be analyzed
- `Termination` defines the termination criteria for every particle

The two, in addition to the existing abstractions for `Particle` and `Field` can be used to specify
novel flow filters. This is accomplished by defining a new trait for the new filter using implementations
for these abstractions.

E.g., for specifying the streamline filter for a general case the following trait can be used

```cpp
template <>
struct FlowTraits<Streamline>
{
  using ParticleType    = vtkm::Particle;
  using TerminationType = vtkm::worklet::flow::NormalTermination;
  using AnalysisType    = vtkm::worklet::flow::StreamlineAnalysis<ParticleType>;
  using ArrayType       = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType       = vtkm::worklet::flow::VelocityField<ArrayType>;
};
```

Similarly, to produce a flow map, the following trait can be used

```cpp
template <>
struct FlowTraits<ParticleAdvection>
{
  using ParticleType = vtkm::Particle;
  using TerminationType = vtkm::worklet::flow::NormalTermination;
  using AnalysisType = vtkm::worklet::flow::NoAnalysis<ParticleType>;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
};
```

These traits are enough for the infrastrucutre to use the correct code paths to produce the desired
result.

Along with changing the existing filters to use this new way of specification of components,
a new filter `WarpXStreamline` has been added to enable streamline analysis for charged particles for
the WarpX simulation.

## Update filters' field map and execute to work on any field type

Several filters implemented their map field by checking for common field
types and interpolated those. Although there was a float fallback to catch
odd component types, there were still a couple of issues. First, it meant
that several types got converted to `vtkm::FloatDefault`, which is often at
odds with how VTK handles it. Second, it does not handle all `Vec` lengths,
so it is still possible to drop fields.

The map field functions for these filters have been changed to support all
possible types. This is done by using the extract component functionality
to get data from any type of array. The following filters have been
updated. In some circumstances where it makes sense, a simple float
fallback is used.

- `CleanGrid`
- `CellAverage`
- `ClipWithField`
- `ClipWithImplicitFunction`
- `Contour`
- `MIRFilter`
- `NDHistogram`
- `ParticleDensityCloudInCell`
- `ParticleDensityNearestGridPoint`
- `PointAverage`
- `Probe`
- `VectorMagnitude`

## Tetrahedralize and Triangulate filters now check if the input is already tetrahedral/triangular

Previously, tetrahedralize/triangulate would blindly convert all the cells to tetrahedra/triangles, even when they were already. Now, the dataset is directly returned if the CellSet is a CellSetSingleType of tetras/triangles, and no further processing is done in the worklets for CellSetExplicit when all shapes are tetras or triangles.

## Added support for `CastAndCallVariableVecField` in `FilterField`

The `FilterField` class provides convenience functions for subclasses to
determine the `ArrayHandle` type for scalar and vector fields. However, you
needed to know the specific size of vectors. For filters that support an
input field of any type, a new form, `CastAndCallVariableVecField` has been
added. This calls the underlying functor with an `ArrayHandleRecombineVec`
of the appropriate component type.

The `CastAndaCallVariableVecField` method also reduces the number of
instances created by having a float fallback for any component type that
does not satisfy the field types.

## New Statistics filter

The statistics filter computes the descriptive statistics of the fields specified by users based on `DescriptiveStatistics`. Users can set `RequiredStatsList` to specify which statistics will be stored in the output data set. The statistics filter supports the distributed memory case based on the vtkmdiy, and the process with rank 0 will return the correct final reduced results.

## Added a `ConvertToPointCloud` filter

This filter takes a `DataSet` and returns a point cloud representation that
has a vertex cell associated with each point in it. This is useful for
filling in a `CellSet` for data that has points but no cells. It is also
useful for operations in which you want to throw away the cell geometry and
operate on the data as a collection of disparate points.

## New Composite Vector filter

The composite vector filter combines multiple scalar fields into a single vector field. Scalar fields are selected as the active input fields, and the combined vector field is set at the output.

## ComputeMoments filter now operates on any scalar field

Previously, the `ComputeMoments` filter only operated on a finite set of
array types as its input field. This included a prescribed list of `Vec`
sizes for the input. The filter has been updated to use more generic
interfaces to the field's array (and float fallback) to enable the
computation of moments on any type of scalar field.

## Continuous Scatterplot filter

This new filter, designed for bi-variate analysis, computes the continuous scatter-plot of a 3D mesh for two given scalar point fields.

The continuous scatter-plot is an extension of the discrete scatter-plot for continuous bi-variate analysis. The output dataset consists of triangle-shaped cells, whose coordinates on the 2D plane represent respectively the values of both scalar fields. The point centered scalar field generated on the triangular mesh quantifies the density of values in the data domain.

This VTK-m implementation is based on the algorithm presented in the paper "Continuous Scatterplots" by S. Bachthaler and D. Weiskopf.

## Support any cell type in MIR filter

Previously, the MIR filter ran a check the dimensionality of the cells in
its input data set to make sure they conformed to the algorithm. The only
real reason this was necessary is because the `MeshQuality` filter can only
check the size of either area or volume, and it has to know which one to
check. However, the `CellMeasures` filter can compute the sizes of all
types of cells simultaneously (as well as more cell types). By using this
filter, the MIR filter can skip the cell type checks and support more mesh
types.

## Get the 3D index from a BoundaryState in WorkletPointNeighborhood

There are occasions when you need a worklet to opeate on 2D or 3D indices.
Most worklets operate on 1D indices, which requires recomputing the 3D
index in each worklet instance. A workaround is to use a worklet that does
a 3D scheduling and pull the working index from that.

The problem was that there was no easy way to get this 3D index. To provide
this option, a feature was added to the `BoundaryState` class that can be
provided by `WorkletPointNeighborhood`.

Thus, to get a 3D index in a worklet, use the `WorkletPointNeighborhood`,
add `Boundary` as an argument to the `ExecutionSignature`, and then call
`GetCenterIndex` on the `BoundaryState` object passed to the worklet
operator.

## Clip now doesn't copy unused points from the input to the output

Previously, clip would just copy all the points and point data from the input to the output,
and only append the new points. This would affect the bounds computation of the result.
If the caller wanted to remove the unused points, they had to run the CleanGrid filter
on the result.

With this change, clip now keeps track of which inputs are actually part of the output
and copies only those points.

## Implement Flying Edges for structured cellsets with rectilinear and curvilinear coordinates

When Flying Edges was introduced to compute contours of a 3D structured cellset, it could only process uniform coordinates. This limitation is now lifted : an alternative interpolation function can be used in the fourth pass of the algorithm in order to support rectilinear and curvilinear coordinate systems.

Accordingly, the `Contour` filter now calls `ContourFlyingEdges` instead of `ContourMarchingCells` for these newly supported cases.

## Added a `HistSampling` filter

This filter assumes the field data are point clouds. It samples the field data according to its importance level. The importance level (sampling rate) is computed based on the histogram. The rarer values can provide more importance. More details can be found in the following paper. “In Situ Data-Driven Adaptive Sampling for Large-scale Simulation Data Summarization”, Ayan Biswas, Soumya Dutta, Jesus Pulido, and James Ahrens, In Situ Infrastructures for Enabling Extreme-scale Analysis and Visualization (ISAV 2018), co-located with Supercomputing 2018.

## New Shrink filter

The Shrink filter shrinks the cells of a DataSet towards their centroid, computed as the average position of the cell points. This filter disconnects the cells, duplicating the points connected to multiple cells. The resulting CellSet is always an `ExplicitCellSet`.

## Fix interpolation of cell fields with flying edges

The flying edges algorithm (used when contouring uniform structured cell
sets) was not interpolating cell fields correctly. There was an indexing
issue where a shortcut in the stepping was not incrementing the cell index.

## Fix degenerate cell removal

There was a bug in `CleanGrid` when removing degenerate polygons where it
would not detect if the first and last point were the same. This has been
fixed.

There was also an error with function overloading that was causing 0D and
3D cells to enter the wrong computation for degenerate cells. This has also
been fixed.


# Control Environment

## Added a reader for VisIt files

A VisIt file is a text file that contains the path and filename of a number of VTK files. This provides a convenient way to load `vtkm::cont::PartitionedDataSet` data from VTK files. The first line of the file is the keyword `!NBLOCKS <N>` that specifies the number of VTK files to be read.

## Fixed operator for IteratorFromArrayPortal

There was an error in `operator-=` for `IteratorFromArrayPortal` that went
by unnoticed. The operator is fixed and regression tests for the operators
has been added.

## Implemented `VecTraits` class for all types

The `VecTraits` class allows templated functions, methods, and classes to
treat type arguments uniformly as `Vec` types or to otherwise differentiate
between scalar and vector types. This only works for types that `VecTraits`
is defined for.

The `VecTraits` templated class now has a default implementation that will
be used for any type that does not have a `VecTraits` specialization. This
removes many surprise compiler errors when using a template that, unknown
to you, has `VecTraits` in its implementation.

One potential issue is that if `VecTraits` gets defined for a new type, the
behavior of `VecTraits` could change for that type in backward-incompatible
ways. If `VecTraits` is used in a purely generic way, this should not be an
issue. However, if assumptions were made about the components and length,
this could cause problems.

Fixes #589.

# Build

## Require Kokkos 3.7

The minimum version of Kokkos supported is now set to Kokkos 3.7. This is
to synchronize with the development of the Kokkos team.

## Sped up compilation of ArrayRangeCompute.cxx

The file `ArrayRangeCompute.cxx` was taking a long time to compile with
some device compilers. This is because it precompiles the range computation
for many types of array structures. It thus compiled the same operation
many times over.

The new implementation compiles just as many cases. However, the
compilation is split into many different translation units using the
instantiations feature of VTK-m's configuration. Although this rarely
reduces the overall CPU time spent during compiling, it prevents parallel
compiles from waiting for this one build to complete. It also avoids
potential issues with compilers running out of resources as it tries to
build a monolithic file.

## Kokkos atomic functions switched to use desul library

Kokkos 4 switches from their interal library based off of desul to using desul
directly.  This removes VTK-m's dependency on the Kokkos internal
implementation (Kokkos::Impl) to using desul directly.

## Added Makefile contract tests

Added Makefile contract tests to ensure that the VTK-m smoke test example
application can be built and run using a Makefile against a VTK-m install tree.
This will help users who use bare Make as their build system. Additionally,
fixed both the VTK-m pkg-config `vtkm.pc` and the `vtkm_config.mk` file to
ensure that both files are correctly generated and added CI coverage to ensure
that they are always up-to-date and correct. This improves support for users
who use bare Make as their build system, and increases confidence in the
correctness of both the VTK-m pkg-config file `vtkm.pc` and of the Makefile
`vtkm_config.mk`.

You can run these tests with: `ctest -R smoke_test`

# Others

## Clarified license of test data

The VTK-m source comes distributed with several data files used for
regression testing. Some of these are generated specifically by VTK-m
developers and are released as part of the VTK-m license, but some come
from external sources. For those that come from external sources, we have
clarified the license and attribution of those files. In particular, the
following files originate from external sources.

- **internet.egr**: Distributed as part of a graph data set paper. The
  license of this data is compatible with VTK-m's license. The file is
  placed in the third-party data directory and the information has been
  updated to clearly document the correct license for this data.
- **example.vtk** and **example_temp.bov**: Distributed as part of the
  VisIt tutorials. This data is provided under the VisIt license (per Eric
  Brugger), which is compatible with VTK-m's license. The files are moved
  to the third-party data directory and the license and attribution is
  clarified. (These files were previously named "noise" but were changed to
  match the VisIt tutorial files they came from.)
- **vanc.vtk** Data derived from a digital elevation map of Vancouver that
  comes from GTOPO30. This data is in the public domain, so it is valid for
  us to use, modify, and redistribute the data under our license.

The fishtank and fusion/magField datasets were removed. These are standard
flow testing data sets that are commonly distributed. However, we could not
track down the original source and license, so to be cautious these data
sets have been removed and replaced with some generated in house.

For some of the other data sets, we have traced down the original author
and verified that they propery contribute the data to VTK-m and agree to
allow it to be distributed under VTK-m's license. Not counting the most
trivial examples, here are the originators of the non-trivial data
examples.

```
- **5x6\_&\_MC*.ctm*\* and **8x9test_HierarchicalAugmentedTree*.dat*\*: Hamish
  Carr
- **warpXfields.vtk** and **warpXparticles.vtk**: Axel Huebl
- **amr_wind_flowfield.vtk**: James Kress
- **DoubleGyre*.vtk*\*: James Kress
- **venn250.vtk**: Abhishek Yenpure
- **wedge_cells.vtk**: Chris Laganella
- **kitchen.vtk**: Copyright owned by Kitware, Inc. (who shares the
  copyright of VTK-m)
```
