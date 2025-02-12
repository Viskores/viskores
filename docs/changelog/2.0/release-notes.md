VTK-m 2.0 Release Notes
=======================

# Table of Contents

1. [Core](#Core)
  - Added modules to the build system
  - Remove deprecated features from VTK-m
2. [ArrayHandle](#ArrayHandle)
  - Support providing a Token to ReadPortal and WritePortal
3. [Control Environment](#Control-Environment)
  - Coordinate systems are stored as Fields
  - Check to make sure that the fields in a DataSet are the proper length
  - Change name of method to set the cell ghost levels in a DataSet
  - Automatically make the fields with the global cell ghosts the cell ghosts
  - Particle class members are hidden
  - Allow FieldSelection to simultaneously include and exclude fields
  - New partitioned cell locator class
  - Fix reading global ids of permuted cells
  - Setting source parameters is more clear
  - Attach compressed ZFP data as WholeDataSet field
4. [Execution Environment](#Execution-Environment)
  - Removed ExecutionWholeArray class
  - Add initial support for aborting execution
5. [Worklets and Filters](#Worklets-and-Filters)
  - Correct particle density filter output field
  - Rename NewFilter base classes to Filter
  - Fix handling of cell fields in Tube filter
  - Fix setting fields to pass in Filter when setting mode
  - Respect Filter::PassCoordinateSystem flag in filters creating coordinate systems.
6. [Build](#Build)
  - More performance test options
  - Output complete list of libraries for external Makefiles
  - VTK-m namespace for its exported CMake targets
7. [Other](#Other)
  - Expose the Variant helper class
  - Fix VTKM_LOG_SCOPE
  - Clarify field index ordering in Doxygen

# Core

## Added modules to the build system

VTK-m libraries and other targets can now be built as modules. The
advantage of modules is that you can selectively choose which
modules/libraries will be built. This makes it easy to create a more
stripped down compile of VTK-m. For example, you might want a reduced set
of libraries to save memory or you might want to turn off certain libraries
to save compile time.

The module system will automatically determine dependencies among the
modules. It is capable of weakly turning off a module where it will still
be compiled if needed. Likewise, it is capable of weakly turning on a
module where the build will still work if it cannot be created.

The use of modules is described in the `Modules.md` file in the `docs`
directory of the VTK-m source.

## Remove deprecated features from VTK-m

With the major revision 2.0 of VTK-m, many items previously marked as
deprecated were removed. If updating to a new version of VTK-m, it is
recommended to first update to VTK-m 1.9, which will include the deprecated
features but provide warnings (with the right compiler) that will point to
the replacement code. Once the deprecations have been fixed, updating to
2.0 should be smoother.

# ArrayHandle

## Support providing a Token to ReadPortal and WritePortal

When managing portals in the execution environment, `ArrayHandle` uses the
`Token` object to ensure that the memory associated with a portal exists
for the length of time that it is needed. This is done by creating the
portal with a `Token` object, and the associated portal objects are
guaranteed to be valid while that `Token` object exists. This is supported
by essentially locking the array from further changes.

`Token` objects are typically used when creating a control-side portal with
the `ReadPortal` or `WritePortal`. This is not to say that a `Token` would
not be useful; a control-side portal going out of scope is definitely a
problem. But the creation and destruction of portals in the control
environment is generally too much work for the possible benefits.

However, under certain circumstances it could be useful to use a `Token` to
get a control-side portal. For example, if the `PrepareForExecution` method
of an `ExecutionObjectBase` needs to fill a small `ArrayHandle` on the
control side to pass to the execution side, it would be better to use the
provided `Token` object when doing so. This change allows you to optionally
provide that `Token` when creating these control-side portals.

# Control Environment

## Coordinate systems are stored as Fields

Previously, `DataSet` managed `CoordinateSystem`s separately from `Field`s.
However, a `CoordinateSystem` is really just a `Field` with some special
attributes. Thus, coordinate systems are now just listed along with the
rest of the fields, and the coordinate systems are simply strings that
point back to the appropriate field. (This was actually the original
concept for `DataSet`, but the coordinate systems were separated from
fields for some now obsolete reasons.)

This change should not be very noticeable, but there are a few consequences
that should be noted.

1. The `GetCoordinateSystem` methods no longer return a reference to a
   `CoordinateSystem` object. This is because the `CoordinateSystem` object
   is made on the fly from the field.
2. When mapping fields in filters, the coordinate systems get mapped as
   part of this process. This has allowed us to remove some of the special
   cases needed to set the coordinate system in the output.
3. If a filter is generating a coordinate system in a special way
   (different than mapping other point fields), then it can use the special
   `CreateResultCoordinateSystem` method to attach this custom coordinate
   system to the output.
4. The `DataSet::GetCoordinateSystems()` method to get a `vector<>` of all
   coordinate systems is removed. `DataSet` no longer internally has this
   structure. Although it could be built, the only reason for its existence
   was to support passing coordinate systems in filters. Now that this is
   done automatically, the method is no longer needed.

## Check to make sure that the fields in a DataSet are the proper length

It is possible in a `DataSet` to add a point field (or coordinate system)
that has a different number of points than reported in the cell set.
Likewise for the number of cells in cell fields. This is very bad practice
because it is likely to lead to crashes in worklets that are expecting
arrays of an appropriate length.

Although `DataSet` will still allow this, a warning will be added to the
VTK-m logging to alert users of the inconsistency introduced into the
`DataSet`. Since warnings are by default printed to standard error, users
are likely to see it.

## Change name of method to set the cell ghost levels in a DataSet

Previously, the method was named `AddGhostCellField`. However, only one
ghost cell field can be marked at a time, so `SetGhostCellField` is more
appropriate.

## Automatically make the fields with the global cell ghosts the cell ghosts

Previously, if you added a cell field to a `DataSet` with a name that was
the same as that returned from `GetGlobalCellFieldName`, it was still only
recognized as a normal field. Now, that field is automatically recognized
as the cell ghost levels (unless the global cell field name is changed or
a different field is explicitly set as the cell ghost levels).

## Particle class members are hidden

The member variables of the `vtkm::Particle` classes are now hidden. This
means that external code will not be directly able to access member
variables like `Pos`, `Time`, and `ID`. Instead, these need to be retrieved
and changed through accessor methods.

This follows standard C++ principles. It also helps us future-proof the
classes. It means that we can provide subclasses or alternate forms of
`Particle` that operate differently. It also makes it possible to change
interfaces while maintaining a deprecated interface.

## Allow FieldSelection to simultaneously include and exclude fields

The basic use of `FieldSelection` is to construct the class with a mode
(`None`, `Any`, `Select`, `Exclude`), and then specify particular fields
based off of this mode. This works fine for basic uses where the same code
that constructs a `FieldSelection` sets all the fields.

But what happens, for example, if you have code that takes an existing
`FieldSelection` and wants to exclude the field named `foo`? If the
`FieldSelection` mode happens to be anything other than `Exclude`, the code
would have to go through several hoops to construct a new `FieldSelection`
object with this modified selection.

To make this case easier, `FieldSelection` now has the ability to specify
the mode independently for each field. The `AddField` method now has an
optional mode argument the specifies whether the mode for that field should
be `Select` or `Exclude`.

In the example above, the code can simply add the `foo` field with the
`Exclude` mode. Regardless of whatever state the `FieldSelection` was in
before, it will now report the `foo` field as not selected.

## New partitioned cell locator class

A new version of a locator, `CellLocatorParitioned`, is now available. This version of a
locator takes a `PartitionedDataSet` and builds a structure that will find the partition Ids and
cell Ids for the input array of locations. It runs CellLocatorGeneral for each partition. We 
expect multiple hits and only return the first one (lowest partition Id) where the detected cell 
is of type REGULAR (no ghost, not blanked) in the vtkGhostType array. If this array does not 
exist in a partition, we assume that all cells are regular.

vtkm::cont::CellLocatorPartitioned produces an Arrayhandle of the size of the number of 
partitions filled with the execution objects of CellLocatorGeneral. It further produces an 
Arrayhandle filled with the ReadPortals of the vtkGhost arrays to then select the non-blanked 
cells from the potentially multiple detected cells on the different partitions. Its counterpart 
on the exec side, vtkm::exec::CellLocatorPartitioned, contains the actual FindCell function.

## Fix reading global ids of permuted cells

The legacy VTK reader sometimes has to permute cell data because some VTK
cells are not directly supported in VTK-m. (For example, triangle strips
are not supported. They have to be converted to triangles.)

The global and pedigree identifiers were not properly getting permuted.
This is now fixed.

## Setting source parameters is more clear

Originally, most of the sources used constructor parameters to set the
various options of the source. Although convenient, it was difficult to
keep track of what each parameter meant. To make the code more clear,
source parameters are now set with accessor functions (e.g.
`SetPointDimensions`). Although this makes code more verbose, it helps
prevent mistakes and makes the changes more resilient to future changes.

## Attach compressed ZFP data as WholeDataSet field

Previously, point fields compressed by ZFP were attached as point fields
on the output. However, using them as a point field would cause
problems. So, instead attache them as `WholeDataSet` fields.

Also fixed a problem where the 1D decompressor created an output of the
wrong size.

# Execution Environment

## Removed ExecutionWholeArray class

`ExecutionWholeArray` is an archaic class in VTK-m that is a thin wrapper
around an array portal. In the early days of VTK-m, this class was used to
transfer whole arrays to the execution environment. However, now the
supported method is to use `WholeArray*` tags in the `ControlSignature` of
a worklet.

Nevertheless, the `WholeArray*` tags caused the array portal transferred to
the worklet to be wrapped inside of an `ExecutionWholeArray` class. This
is unnecessary and can cause confusion about the types of data being used.

Most code is unaffected by this change. Some code that had to work around
the issue of the portal wrapped in another class used the `GetPortal`
method which is no longer needed (for obvious reasons). One extra feature
that `ExecutionWholeArray` had was that it provided an subscript operator
(somewhat incorrectly). Thus, any use of '[..]' to index the array portal
have to be changed to use the `Get` method.

## Add initial support for aborting execution

VTK-m now has preliminary support for aborting execution. The per-thread instances of
`RuntimeDeviceTracker` have a functor called `AbortChecker`. This functor can be set using
`RuntimeDeviceTracker::SetAbortChecker()` and cleared by `RuntimeDeviceTracker::ClearAbortChecker()`
The abort checker functor should return `true` if an abort is requested for the thread,
otherwise, it should return `false`.

Before launching a new task, `TaskExecute` calls the functor to see if an abort is requested,
and If so, throws an exception of type `vtkm::cont::ErrorUserAbort`.

Any code that wants to use the abort feature, should set an appropriate `AbortChecker`
functor for the target thread. Then any piece of code that has parts that can execute on
the device should be put under a `try-catch` block. Any clean-up that is required for an
aborted execution should be handled in a `catch` block that handles exceptions of type
`vtkm::cont::ErrorUserAbort`.

The limitation of this implementation is that it is control-side only. The check for abort
is done before launching a new device task. Once execution has begun on the device, there is
currently no way to abort that. Therefore, this feature is only useful for aborting code
that is made up of several smaller device task launches (Which is the case for most
worklets and filters in VTK-m)

# Worklets and Filters

## Correct particle density filter output field

The field being created by `ParticleDensityNearestGridPoint` was supposed
to be associated with cells, but it was sized to the number of points.
Although the number of points will always be more than the number of cells
(so the array will be big enough), having inappropriately sized arrays can
cause further problems downstream.

## Rename NewFilter base classes to Filter

During the VTK-m 1.8 and 1.9 development, the filter infrastructure was
overhauled. Part of this created a completely new set of base classes. To
avoid confusion with the original filter base classes and ease transition,
the new filter base classes were named `NewFilter*`. Eventually after all
filters were transitioned, the old filter base classes were deprecated.

With the release of VTK-m 2.0, the old filter base classes are removed. The
"new" filter base classes are no longer new. Thus, they have been renamed
simply `Filter` (and `FilterField`).

## Fix handling of cell fields in Tube filter

The `Tube` filter wraps a tube of polygons around poly line cells.
During this process it had a strange (and wrong) handling of cell data.
It assumed that each line had an independent field entry for each
segment of each line. It thus had lots of extra code to find the length
and offsets of the segment data in the cell data.

This is simply not how cell fields work in VTK-m. In VTK-m, each cell
has exactly one entry in the cell field array. Even if a polyline has
100 segments, it only gets one cell field value. This behavior is
consistent with how VTK treats cell field arrays.

The behavior the `Tube` filter was trying to implement was closer to an
"edge" field. However, edge fields are currently not supported in VTK-m.
The proper implementation would be to add edge fields to VTK-m. (This
would also get around some problems with the implementation that was
removed here when mixing polylines with other cell types and degenerate
lines.)

## Fix setting fields to pass in `Filter` when setting mode

The `Filter` class has several version of the `SetFieldsToPass` method that
works in conjunction with the `FieldSelection` object to specify which
fields are mapped. For example, the user might have code like this to pass
all fields except those named `pointvar` and `cellvar`:

``` cpp
    filter.SetFieldsToPass({ "pointvar", "cellvar" },
                           vtkm::filter::FieldSelection::Mode::Exclude);
```

This previously worked by implicitly creating a `FieldSelection` object
using the `std::initializer_list` filled with the 2 strings. This would
then be passed to the `SetFieldsToPass` method, which would capture the
`FieldSelection` object and change the mode.

This stopped working in a recent change to `FieldSelection` where each
entry is given its own mode. With this new class, the `FieldSelection`
constructor would capture each field in the default mode (`Select`) and
then change the default mode to `Exclude`. However, the already set modes
kept their `Select` status, which is not what is intended.

This behavior is fixed by adding `SetFieldToPass` overloads that capture
both the `initializer_list` and the `Mode` and then constructs the
`FieldSelection` correctly.

## Respect `Filter::PassCoordinateSystem` flag in filters creating coordinate systems

The `Filter` class has a `PassCoordinateSystem` flag that specifies whether
coordinate systems should be passed regardless of whether the associated
field is passed. However, if a filter created its output with the
`CreateResultCoordinateSystem` method this flag was ignored, and the
provided coordinate system was always passed. This might not be what the
user intended, so this method has been fixed to first check the
`PassCoordinateSystem` flag before setting the coordinates on the output.

# Build

## More performance test options

More options are available for adding performance regression tests. These
options allow you to pass custom options to the benchmark test so that you
are not limited to the default values. They also allow multiple tests to be
created from the same benchmark executable. Separating out the benchmarks
allows the null hypothesis testing to better catch performance problems
when only one of the tested filters regresses. It also allows passing
different arguments to different benchmarks.

## Output complete list of libraries for external Makefiles

There is a Makefile include, `vtkm_config.mk`, and a package include,
`vtkm.pc`, that are configured so that external programs that do not use
CMake have a way of importing VTK-m's configuration. However, the set of
libraries was hardcoded. In particular, many of the new filter libraries
were missing.

Rather than try to maintain this list manually, the new module mechanism
in the CMake configuration is used to get a list of libraries built and
automatically build these lists.

## VTK-m namespace for its exported CMake targets

VTK-m exported CMake targets are now prefixed with the `vtkm::` namespace.

### What it means for VTK-m users

VTK-m users will now need to prepend a `vtkm::` prefix when they refer to a
VTK-m CMake target in their projects as shown below:

```
add_executable(example example.cxx)
## Before:
target_link_libraries(example vtkm_cont vtkm_rendering)
## Now:
target_link_libraries(example vtkm::cont vtkm::rendering)
```

For compatibility purposes we still provide additional exported targets with the
previous naming scheme, in the form of `vtkm_TARGET`,  when VTK-m is found
using:

```
## With any version less than 2.0
find_package(VTK-m 1.9)

add_executable(example example.cxx)
## This is still valid
target_link_libraries(example vtkm_cont vtkm_rendering)
```

Use with care since we might remove those targets in future releases.

### What it means for VTK-m developers

While VTK-m exported targets are now prefixed with the `vtkm::` prefix, internal
target names are still in the form of `vtkm_TARGET`.

To perform this name transformation in VTK-m targets a new CMake function has
been provided that decorates the canonical `install` routine. Use this functions
instead of `install` when creating new `VTK-m` targets, further information can
be found at the `vtkm_install_targets` function header at
`CMake/VTKmWrappers.cmake`.

# Other

## Expose the Variant helper class

For several versions, VTK-m has had a `Variant` templated class. This acts
like a templated union where the object will store one of a list of types
specified as the template arguments. (There are actually 2 versions for the
control and execution environments, respectively.)

Because this is a complex class that required several iterations to work
through performance and compiler issues, `Variant` was placed in the
`internal` namespace to avoid complications with backward compatibility.
However, the class has been stable for a while, so let us expose this
helpful tool for wider use.

## Fix VTKM_LOG_SCOPE

The `VTKM_LOG_SCOPE` macro was not working as intended. It was supposed to
print a log message immediately and then print a second log message when
leaving the scope along with the number of seconds that elapsed between the
two messages.

This was not what was happening. The second log message was being printed
immediately after the first. This is because the scope was taken inside of
the `LogScope` method. The macro has been rewritten to put the tracking in
the right scope.

## Clarify field index ordering in Doxygen

The fields in a `DataSet` are indexed from `0` to `GetNumberOfFields() - 1`.
It is natural to assume that the fields will be indexed in the order that
they are added, but they are not. Rather, the indexing is arbitrary and can
change any time a field is added to the dataset.

To make this more clear, Doxygen documentation is added to the `DataSet`
methods to inform users to not make any assumptions about the order of
field indexing.
