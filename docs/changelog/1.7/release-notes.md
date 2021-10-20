VTK-m 1.7 Release Notes
=======================

# Table of Contents
1. [Core](#Core)
  - Add ability to convert fields to known types
  - Consolidate count-to-offset algorithms
  - Make field names from sources more descriptive
2. [ArrayHandle](#ArrayHandle)
  - Added `ArrayCopyShallowIfPossible`
  - Added copy methods to `UnknownArrayHandle`
  - Allow a `const ArrayHandle` to be reallocated
  - Compile `ArrayGetValues` implementation in a library
  - Deprecated `VariantArrayHandle`
  - Improve type reporting in `UnknownArrayHandle`
[4](4). [Control Environment](#Control-Environment)
  - Compile reverse connectivity builder into vtkm_cont library
5. [Execution Environment](#Execution-Environment)
  - Remove unbounded recursion
6. [Worklets and Filters](#Worklets-and-Filters)
  - [GenerateIds](GenerateIds) filter
  - Support scatter/mask for CellSetExtrude
  - Adding ability to use cell-centered velocity fields for particle advection
  - Probe always generates point fields
7. [Build](#Build)
  - Filters instantiation generator
  - Move .cxx from DEVICE_SOURCES to SOURCES in vktm IO library
8. [Other](#Other)
  - Enable `TypeToString` for `type_info`
  - Skip library versions
  - Support writing binary files to legacy VTK files

# Core

## Add ability to convert fields to known types ##

In VTK-m we have a constant tension between minimizing the number of
types we have to compile for (to reduce compile times and library size)
and maximizing the number of types that our filters support.
Unfortunately, if you don't compile a filter for a specific array type
(value type and storage), trying to run that filter will simply fail.

To compromise between the two, added methods to `DataSet` and `Field`
that will automatically convert the data in the `Field` arrays to a type
that VTK-m will understand. Although this will cause an extra data copy,
it will at least prevent the program from failing, and thus make it more
feasible to reduce types.

## Consolidate count-to-offset algorithms ##

For no particularly good reason, there were two functions that converted
and array of counts to an array of offsets: `ConvertNumComponentsToOffsets`
and `ConvertNumIndicesToOffsets`. These functions were identical, except
one was defined in `ArrayHandleGroupVecVariable.h` and the other was
defined in `CellSetExplicit.h`.

These two functions have been consolidated into one (which is now called
`ConvertNumComponentsToOffsets`). The consolidated function has also been
put in its own header file: `ConvertNumComponentsToOffsets.h`.

Normally, backward compatibility would be established using deprecated
features. However, one of the things being worked on is the removal of
device-specific code (e.g. `vtkm::cont::Algorithm`) from core classes like
`CellSetExplicit` so that less code needs to use the device compiler
(especially downstream code).

`ConvertNumComponentsToOffsets` has also been changed to provide a
pre-compiled version for common arrays. This helps with the dual goals of
compiling less device code and allowing data set builders to not have to
use the device compiler. For cases where you need to compile
`ConvertNumComponentsToOffsets` for a different kind of array, you can use
the internal `ConvertNumComponentsToOffsetsTemplate`.

Part of this change removed unnecessary includes of `Algorithm.h` in
`ArrayHandleGroupVecVariable.h` and `CellSetExplicit.h`. This header had to
be added to some classes that were not including it themselves.

## Make field names from sources more descriptive ##

The VTK-m sources (like `Oscillator`, `Tangle`, and `Wavelet`) were all
creating fields with very generic names like `pointvar` or `scalars`. These
are very unhelpful names as it is impossible for downstream processes to
identify the meaning of these fields. Imagine having these data saved to a
file and then a different person trying to identify what they mean. Or
imagine dealing with more than one such source at a time and trying to
manage fields with similar or overlapping names.

The following renames happened:

  * `Oscillator`: `scalars` -> `oscillating`
  * `Tangle`: `pointvar` -> `tangle`
  * `Wavelet`: `scalars` -> `RTData` (matches VTK source)

# ArrayHandle

## Added `ArrayCopyShallowIfPossible` ##

Often times you have an array of an unknown type (likely from a data set),
and you need it to be of a particular type (or can make a reasonable but
uncertain assumption about it being a particular type). You really just
want a shallow copy (a reference in a concrete `ArrayHandle`) if that is
possible.

`ArrayCopyShallowIfPossible` pulls an array of a specific type from an
`UnknownArrayHandle`. If the type is compatible, it will perform a shallow
copy. If it is not possible, a deep copy is performed to get it to the
correct type.

## Added copy methods to `UnknownArrayHandle` ##

`vtkm::cont::UnknownArrayHandle` now provides a set of method that allows
you to copy data from one `UnknownArrayHandle` to another. The first
method, `DeepCopyFrom`, takes a source `UnknownArrayHandle` and deep copies
the data to the called one. If the `UnknownArrayHandle` already points to a
real `ArrayHandle`, the data is copied into that `ArrayHandle`. If the
`UnknownArrayHandle` does not point to an existing `ArrayHandle`, then a
new `ArrayHandleBasic` with the same value type as the source is created
and copied into.

The second method, `CopyShallowIfPossibleFrom` behaves similarly to
`DeepCopyFrom` except that it will perform a shallow copy if possible. That
is, if the target `UnknownArrayHandle` points to an `ArrayHandle` of the
same type as the source `UnknownArrayHandle`, then a shallow copy occurs
and the underlying `ArrayHandle` will point to the source. If the types
differ, then a deep copy is performed. If the target `UnknownArrayHandle`
does not point to an `ArrayHandle`, then the behavior is the same as the
`=` operator.

One of the intentions of these new methods is to allow you to copy arrays
without using a device compiler (e.g. `nvcc`). Calling `ArrayCopy` requires
you to include the `ArrayCopy.h` header file, and that in turn requires
device adapter algorithms. These methods insulate you from these.

## Allow a `const ArrayHandle` to be reallocated ##

Previously, the `Allocate` method of `ArrayHandle` was _not_ declared as
`const`. Likewise, the methods that depended on `Allocate`, namely
`ReleaseResources` and `PrepareForOutput` were also not declared `const`.
The main consequence of this was that if an `ArrayHandle` were passed as a
constant reference argument to a method (e.g. `const ArrayHandle<T>& arg`),
then the array could not be reallocated.

This seems right at first blush. However, we have changed these methods to
be `const` so that you can in fact reallocate the `ArrayHandle`. This is
because the `ArrayHandle` is in principle a pointer to an array pointer.
Such a structure in C will allow you to change the pointer to the array,
and so in this context it makes sense for `ArrayHandle` to support that as
well.

Although this distinction will certainly be confusing to users, we think
this change is correct for a variety of reasons.

  1. This change makes the behavior of `ArrayHandle` consistent with the
     behavior of `UnknownArrayHandle`. The latter needed this behavior to
     allow `ArrayHandle`s to be passed as output arguments to methods that
     get automatically converted to `UnknownArrayHandle`.
  2. Before this change, a `const ArrayHandle&` was still multible is many
     way. In particular, it was possible to change the data in the array
     even if the array could not be resized. You could still call things
     like `WritePortal` and `PrepareForInOut`. The fact that you could
     change it for some things and not others was confusing. The fact that
     you could call `PrepareForInOut` but not `PrepareForOutput` was doubly
     confusing.
  3. Passing a value by constant reference should be the same, from the
     calling code's perspective, as passing by value. Although the function
     can change an argument passed by value, that change is not propogated
     back to the calling code. However, in the case of `ArrayHandle`,
     calling by value would allow the array to be reallocated from the
     calling side whereas a constant reference would prevent that. This
     change makes the two behaviors consistent.
  4. The supposed assurance that the `ArrayHandle` would not be reallocated
     was easy to break even accidentally. If the `ArrayHandle` was assigned
     to another `ArrayHandle` (for example as a class' member or wrapped
     inside of an `UnknownArrayHandle`), then the array was free to be
     reallocated.

## Compile `ArrayGetValues` implementation in a library ##

Previously, all of the `ArrayGetValue` implementations were templated
functions that had to be built by all code that used it. That had 2
negative consequences.

1. The same code that scheduled jobs on any device had to be compiled many
   times over.
2. Any code that used `ArrayGetValue` had to be compiled with a device
   compiler. If you had non-worklet code that just wanted to get a single
   value out of an array, that was a pain.

To get around this problem, an `ArrayGetValues` function that takes
`UnknownArrayHandle`s was created. The implementation for this function is
compiled into a library. It uses `UnknownArrayHandle`'s ability to extract
a component of the array with a uniform type to reduce the number of code
paths it generates. Although there are still several code paths, they only
have to be computed once. Plus, now any code can include `ArrayGetValues.h`
and still use a basic C++ compiler.

## Deprecated `VariantArrayHandle` ##

`VaraintArrayHandle` has been replaced by `UnknownArrayHandle` and
`UncertainArrayHandle`. Officially made `VariantArrayHandle` deprecated and
point users to the new implementations.

## Improve type reporting in `UnknownArrayHandle` ##

Added features with reporting types with `UnknownArrayHandle`. First, added
a method named `GetArrayTypeName` that returns a string containing the type
of the contained array. There were already methods `GetValueType` and
`GetStorageType`, but this provides a convenience to get the whole name in
one go.

Also improved the reporting when an `AsArrayHandle` call failed. Before,
the thrown method just reported that the `UnknownArrayHandle` could not be
converted to the given type. Now, it also reports the type actually held by
the `UnknownArrayHandle` so the user can better understand why the
conversion failed.

# Control Environment

## Compile reverse connectivity builder into vtkm_cont library ##

Because `CellSetExplicit` is a templated class, the implementation of
most of its features is part of the header files. One of the things that
was included was the code to build the reverse connectivity links. That
is, it figured out which cells were incident on each point using the
standard connections of which points comprise which cells.

Of course, building these links is non-trivial, and it used multiple
DPPs to engage the device. It meant that header had to include the
device adapter algorithms and therefore required a device compiler. We
want to minimize this where possible.

To get around this issue, a non-templated function was added to find the
reverse connections of a `CellSetExplicit`. It does this by passing in
`UnknownArrayHandle`s for the input arrays. (The output visit-points-
with-cells arrays are standard across all template instances.) The
implementation first iterates over all `CellSetExplicit` versions in
`VTKM_DEFAULT_CELL_SETS` and attempts to retrieve arrays of those types.
In the unlikely event that none of these arrays work, it copies the data
to `ArrayHandle<vtkm::Id>` and uses those.

# Execution Environment

## Remove unbounded recursion ##

GPU device compilers like to determine the stack size needed for a called
kernel. This is only possible if there is no recursive function calls on
the device or at least recursive calls where the termination cannot be
found at compile time.

Device compilers do not particularly like that. We have been getting around
this with CUDA by turning of warnings about stack sizes and setting a large
stack size during a call (which works but is dangerous). More restrictive
devices might not allow recursive calls at all.

To fix this, we will avoid recursive calls in execution environment
(device) code. All such warnings are turned on.

Because of this, we also should not have to worry about lengthening the
stack size, so that code is also removed.

# Worklets and Filters

## GenerateIds filter ##

This filter adds a pair of fields to a `DataSet` which mirror the
indices of the points and cells, respectively. These fields are useful
for tracking the provenance of the elements of a `DataSet` as it gets
manipulated by the filters. It is also convenient for adding indices to
operations designed for fields and for testing purposes.

## Support scatter/mask for CellSetExtrude ##

Scheduling topology map workets for `CellSetExtrude` always worked, but the
there were indexing problems when a `Scatter` or a `Mask` was used. This
has been corrected, and now `Scatter`s and `Mask`s are supported on
topology maps on `CellSetExtrude`.

## Adding ability to use cell-centered velocity fields for particle advection ##

Vector fields for particle advection are not always nodal,; e.g., AMR-Wind uses
zonal vector fields to store velocity information. Previously, VTK-m filters
only supported particle advection in nodal vector fields. With this change, VTK-m
will support zonal vector fields. Users do not need to worry about changing the
way they specify inputs to the flow visualization filters. However, if users use
the particle advection worklets, they'll need to specify the associativity for
their vector fields.

## Probe always generates point fields ##

Previously, the `probe` filter, when probing the input's cell fields, would
store the result as the output's cell field. This is a bug since the probing is
done at the geometry's point locations, and the output gets its structure from
the `geometry`.

This behaviour is fixed in this release. Now, irrespective of the type of the
input field being probed, the result field is always a point field.

```
vtkm::cont::Field field = dataset.GetField("velocity");
vtkm::cont::Field::Association assoc = field.GetAssociation();

using FieldArray = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using FieldType  = vtkm::worklet::particleadvection::VelocityField<FieldType>;

FieldArray data;
field.GetData().AsArrayHandle<FieldArray>(data);

// Use this field to pass to the GridEvaluators
FieldType velocities(data, assoc);
```

# Build

## Filters instantiation generator ##

It introduces a template instantiation generator. This aims to significantly
reduce the memory usage when building _VTK-m_ filter by effectively splitting
templates instantiations across multiple files.

How this works revolves around automatically instantiating filters template
methods inside transient instantiation files which resides solely in the build
directory. Each of those transient files contains a single explicit template
instantiation.

Here is an example of how to produce an instantiation file.

First, at the filter header file:

```c++

// 1. Include Instantiations header
#include <vtkm/filter/Instantiations.h>

class Contour {
  template <typename T, typename StorageType, typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet&,
                                const vtkm::cont::ArrayHandle<T, StorageType>&,
                                const vtkm::filter::FieldMetadata&,
                                vtkm::filter::PolicyBase<DerivedPolicy>);
};

// 2. Create extern template instantiation and surround with
//    VTKM_INSTANTIATION_{BEGIN,END}
VTKM_INSTANTIATION_BEGIN
extern template vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

```

Later, in its corresponding `CMakeLists.txt` file:

```cmake

vtkm_add_instantiations(ContourInstantiations FILTER Contour)
vtkm_library(
  NAME vtkm_filter_contour
  ...
  DEVICE_SOURCES ${ContourInstantiations}
  )

```

After running the configure step in _CMake_, this will result in the creation of
the following transient file in the build directory:

```c++

#ifndef vtkm_filter_ContourInstantiation0_cxx
#define vtkm_filter_ContourInstantiation0_cxx
#endif

/* Needed for linking errors when no instantiations */
int __vtkm_filter_ContourInstantiation0_cxx;

#include <vtkm/filter/Contour.h>
#include <vtkm/filter/Contour.hxx>

namespace vtkm
{
namespace filter
{

template vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

}
}

#undef vtkm_filter_ContourInstantiation0_cxx

```
## Move .cxx from DEVICE_SOURCES to SOURCES in vktm io library ##

It used to be that every .cxx file which uses ArrayHandle needs to be compiled
by the device compiler. A recent change had removed this restriction.
One exception is that user of ArrayCopy still requires device compiler.
Since most .cxx files in vtkm/io do not use ArrayCopy, they are moved
to SOURCES and are compiled by host compiler.

# Other

## Enable `TypeToString` for `type_info` ##

VTK-m contains a helpful method named `vtkm::cont::TypeToString` that
either takes a type as a template argument or a `std::type_info` object and
returns a human-readable string for that type.

The standard C++ library has an alternate for `std::type_info` named
`std::type_index`, which has the added ability to be used in a container
like `set` or `map`. The `TypeToString` overloads have been extended to
also accept a `std::type_info` and report the name of the type stored in it
(rather than the name of `type_info` itself).


## skip library versions ##

The `VTKm_SKIP_LIBRARY_VERSIONS` variable is now available to skip the SONAME
and SOVERSION fields (or the equivalent for non-ELF platforms).

Some deployments (e.g., Python wheels or Java `.jar` files) do not support
symlinks reliably and the way the libraries get loaded just leads to
unnecessary files in the packaged artifact.

## Support writing binary files to legacy VTK files ##

The legacy VTK file writer writes out in ASCII. This is helpful when a
human is trying to read the file. However, if you have more than a
trivial amount of data, the file can get impractically large. To get
around this, `VTKDataSetWriter` now has a flag that allows you to write
the data in binary format.
