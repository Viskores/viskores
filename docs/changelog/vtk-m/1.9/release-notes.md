VTK-m 1.9 Release Notes
=======================

# Table of Contents

1. [Core](#Core)
- Fix bug with copying invalid variants
- Add Variant::IsType
- Initialize DIY in vtkm::cont::Initialize

2. [ArrayHandle](#ArrayHandle)
- Add test for array and datas that are cleaned up after finalize
- Fix type comparison on OSX
- Allow ArrayHandle to have a runtime selectable number of buffers
- Do not require `VecTraits` for `UnknownArrayHandle` components

3. [Filters](#Filters)
- Old Filter Base Classes are Deprecated
- Divided the mesh quality filter
- Fixed Flying Edges Crash

4. [Build](#Build)
- Added DEVICE_SOURCES to vtkm_unit_tests

5. [Other](#Other)
- Fix bug with voxels in legacy vtk files

# Core

## Fix bug with copying invalid variants

There was a bug where if you attempted to copy a `Variant` that was not
valid (i.e. did not hold an object); a seg fault could happen. This has
been changed to set the target variant to also be invalid.

## Add Variant::IsType

The `Variant` class was missing a way to check the type. You could do it
indirectly using `variant.GetIndex() == variant.GetIndexOf<T>()`, but
having this convenience function is more clear.

## Initialize DIY in vtkm::cont::Initialize

This has the side effect of initialing `MPI_Init` (and will also
call `MPI_Finalize` at program exit). However, if the calling 
code has already called `MPI_Init`, then nothing will happen. 
Thus, if the calling code wants to manage `MPI_Init/Finalize`,
it can do so as long as it does before it initializes VTK-m.

# ArrayHandle

## Add test for array and datas that are cleaned up after finalize

It is the case that arrays might be deallocated from a device after the
device is closed. This can happen, for example, when an `ArrayHandle` is
declared globally. It gets constructed before VTK-m is initialized. This
is OK as long as you do not otherwise use it until VTK-m is initialized.
However, if you use that `ArrayHandle` to move data to a device and that
data is left on the device when the object closes, then the
`ArrayHandle` will be left holding a reference to invalid device memory
once the device is shut down. This can cause problems when the
`ArrayHandle` destructs itself and attempts to release this memory.

The VTK-m devices should gracefully handle deallocations that happen
after device shutdown.

## Fix type comparison on OSX

`UnknownArrayHandle` compares `std::type_index` objects to check whether a
requested type is the same as that held in the array handle. However, it is
possible that different translation units can create different but
equivalent `std::type_info`/`std::type_index` objects. In this case, the
`==` operator might return false for two equivalent types. This can happen
on OSX.

To get around this problem, `UnknownArrayHandle` now does a more extensive
check for `std::type_info` object. It first uses the `==` operator to
compare them (as before), which usually works but can possibly return
`false` when the correct result is `true`. To check for this case, it then
compares the name for the two types and returns `true` iff the two names
are the same.


## Allow ArrayHandle to have a runtime selectable number of buffers

Previously, the number of buffers held by an `ArrayHandle` had to be
determined statically at compile time by the storage. Most of the time this
is fine. However, there are some exceptions where the number of buffers
need to be selected at runtime. For example, the `ArrayHandleRecombineVec`
does not specify the number of components it uses, and it needed a hack
where it stored buffers in the metadata of another buffer, which is bad.

This change allows the number of buffers to vary at runtime (at least at
construction). The buffers were already managed in a `std::vector`. It now
no longer forces the vector to be a specific size. `GetNumberOfBuffers` was
removed from the `Storage`. Instead, if the number of buffers was not
specified at construction, an allocation of size 0 is done to create
default buffers.

The biggest change is to the interface of the storage object methods, which
now take `std::vector` instead of pointers to `Buffer` objects. This adds a
little hassle in having to copy subsets of this `vector` when a storage
object has multiple sub-arrays. But it does simplify some of the
templating.

Other changes to the `Storage` structure include requiring all objects to
include a `CreateBuffers` method that accepts no arguments. This method
will be used by `ArrayHandle` in its default constructor. Previously,
`ArrayHandle` would create the `vector` of `Buffer` objects itself, but it
now must call this method in the `Storage` to do this. (It also has a nice
side effect of allowing the `Storage` to initialize the buffer objects if
necessary. Another change was to remove the `GetNumberOfBuffers` method
(which no longer has meaning).

## Do not require `VecTraits` for `UnknownArrayHandle` components

Whan an `UnknownArrayHandler` is constructed from an `ArrayHandle`, it uses
the `VecTraits` of the component type to construct its internal functions.
This meant that you could not put an `ArrayHandle` with a component type
that did not have `VecTraits` into an `UnknownArrayHandle`.

`UnknownArrayHandle` now no longer needs the components of its arrays to
have `VecTraits`. If the component type of the array does not have
`VecTraits`, it treats the components as if they are a scalar type.

# Filters

## Old Filter Base Classes are Deprecated

In recent versions of VTK-m, a new structure for filter classes was
introduced. All of the existing filters have been moved over to this new
structure, and the old filter class structure has been deprecated.

This is in preparation for changed in VTK-m 2.0, where the old filter
classes will be removed and the new filter classes will have the `New` in
their name removed (so that they become simply `Filter` and `FilterField`).

## Divided the mesh quality filter

The original implementation of the `MeshQuality` filter created one large
kernel with a switch statement that jumped to the code of the metric
actually desired. This is problematic for a couple of reasons. First, it
takes the compiler a long time to optimize for all the inlined cases of a
large kernel. Second, it creates a larger than necessary function that has
to be loaded onto the GPU to execute.

The code was modified to move the switch statement outside of the GPU
kernel. Instead, the routine for each metric is compiled into its own
kernel. For convenience, each routine is wrapped into its own independent
filter (e.g., `MeshQualityArea`, `MeshQualityVolume`). The uber
`MeshQuality` filter still exists, and its use is still encouraged even if
you only need a particular metric. However, internally the switch statement
now occurs on the host to select the appropriate specific filter that loads
a more targeted kernel.

## Fixed Flying Edges Crash

There was a bug in VTK-m's flying edges algorithm (in the contour filter
for uniform structured data) that cause the code to read an index from
uninitialized memory. This in turn caused memory reads from an
inappropriate address that could cause bad values, failed assertions, or
segmentation faults.

The problem was caused by a misidentification of edges at the positive z
boundary. Due to a typo, the z index was being compared to the length in
the y dimension. Thus, the problem would only occur in the case where the y
and z dimensions were of different sizes and the contour would go through
the positive z boundary of the data, which was missing our test cases.

# Build

## Added DEVICE_SOURCES to vtkm_unit_tests

The `vtkm_unit_tests` function in the CMake build now allows you to specify
which files need to be compiled with a device compiler using the
`DEVICE_SOURCES` argument. Previously, the only way to specify that unit
tests needed to be compiled with a device compiler was to use the
`ALL_BACKENDS` argument, which would automatically compile everything with
the device compiler as well as test the code on all backends.
`ALL_BACKENDS` is still supported, but it no longer changes the sources to
be compiled with the device compiler.

# Other

## Fix bug with voxels in legacy vtk files

The legacy VTK file reader for unstructured grids had a bug when reading
cells of type voxel. VTK-m does not support the voxel cell type in
unstructured grids (i.e. explicit cell sets), so it has to convert them to
hexahedron cells. A bug in the reader was mangling the cell array index
during this conversion.
