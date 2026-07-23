VTK-m 1.3 Release Notes
=======================

# Table of Contents
1. [Core](#Core)
    - `OpenMP` Device Adapter
    - Make all worklets dispatches invoke using a `TryExecute`
    - Remove TryExecute from each `filter`
    - `DeviceAdapterTags` are usable for runtime device selection
    - New geometry classes
    - Logging support via loguru.
2. [ArrayHandle](#ArrayHandle)
    - `ArrayHandle` can now take ownership of a user allocated memory location
    - `StorageBasic` now can provide or be provided a delete function
    - `ArrayHandleTransform` works with ExecObject
    - Add `ArrayHandleView` fancy array
    - `ArrayHandleCompositeVector` simplified and made writable.
    - `ArrayHandleExtractComponent` target component is now set at runtime
    - `ArrayHandleSwizzle` component maps are now set at runtime
3. [Control Enviornment](#Control-Enviornment)
    - Interfaces for VTK-m spatial search structures added
    - `vtkm::cont::Algorithm` now can be told which device to use at runtime
    - Support `ExecArg` behavior in `vtkm::cont::Algorithm` methods
    - `vtkm::cont::TryExecuteOnDevice` allows for runtime selection of which device to execute on
    - `vtkm::cont::CellSetExplicit` now caches CellToPoint table when used with Invoke
    - `vtkm::cont::RuntimeDeviceInformation` doesn't require a device template
    - `vtkm::cont::VirtualObjectHandle` can transfer to a device using runtime `DeviceAdapterId` value
    - Add new `vtkm::exec` and `vtkm::cont` execution objects
    - Added a `ReleaseResources` API to CellSet and its derived classes
    - Added a `ReleaseResourcesExecution` API for Field to unload execution resources
    - Added a `ReleaseResourcesExecution` API for CoordinateSystem to unload execution resources
    - Use the strong typed enums for `vtkm::cont::Field`
    - `vtkm::cont::DeviceAdapterId` has becomes a real constexpr type and not an alias to vtkm::UInt8
4. [Execution Enviornment](#Execution-Enviornment)
    - User defined execution objects now usable with runtime selection of device adapter
    - `Dot` function name changed
    - Added float version operations for `vtkm::Math Pi()`
    - `vtkm::Math Pi` functions are now constexpr
    - `CellDerivativeFor3DCell` has a better version for Vec of Vec fields.
    - Add a CUDA-safe `vtkm::Swap` method
5. [Worklets and Filters](#Worklets-and-Filters)
    - Worklets are now asynchronous on CUDA
    - Worklets now execute on CUDA using grid stride loops
    - Scatter class moved to dispatcher
    - VTK-m now supports dispatcher parameters being pointers
    - Histogram filter supports custom types
    - Cell measure functions, worklet, and filter
    - Add a `WaveletGenerator` worklet (e.g. vtkRTAnalyticSource)
    - Add a filter to support Lagrangian analysis capabilities
    - Add a warp vector worklet and filter
    - Add a warp scalar worklet and filter
    - Add a split sharp edge filter
    - Time-varying "oscillator" filter and example
6. [Build](#Build)
    - Build System Redesign and new minimum CMake
    - Add `none` option to `VTKm_CUDA_Architecture`
    - Add a new cmake option: `VTKm_ENABLE_DEVELOPER_FLAGS`
    - Add a new cmake option: `VTKm_INSTALL_ONLY_LIBRARIES`
7. [Other](#Other)
    - Control CUDA managed memory with an environment variable
    - Add support for deferred freeing of CUDA memory
    - Allow variable arguments to `VTKM_TEST_ASSERT`
    - Support `constexpr` and variadic constructor for Vec
    - `vtkm::Vec< vtkm::Vec<T> >` can't be constructed from `vtkm::Vec<U>`
    - Use `std::call_once` to construct singeltons
    - Use `thread_local` in `vtkm::cont::GetGlobalRuntimeDeviceTracker` function if possible
    - Replace `std::random_shuffle` with `std::shuffle`

# Core

## `OpenMP` Device Adapter

A device adapter that leverages `OpenMP 4.0` for threading is now available. The
new adapter is enabled using the CMake option `VTKm_ENABLE_OPENMP` and its
performance is comparable to the TBB device adapter.

Performance comparisions of `OpenMP` against the `TBB` and  `Serial` device
adapters can be found at: https://gitlab.kitware.com/vtk/vtk-m/-/issues/223


## Make all worklets dispatches invoke using a `TryExecute`

Rather than force all dispatchers to be templated on a device adapter,
instead use a TryExecute internally within the invoke to select a device
adapter.

This changes touches quite a bit a code. The first pass of the change
usually does the minimum amount of work, which is to change the
compile-time specification of the device to a run-time call to `SetDevice`
on the dispatcher. Although functionally equivalent, it might mean calling
`TryExecute` within itself.

## Remove TryExecute from each `filter`

The recenet change to dispatchers  has embedded a
`TryExecute` internally within the `Invoke` function of all dispatchers. This
means that it is no longer necessary to specify a device when invoking a
worklet.

Previously, this `TryExecute` was in the filter layer. The filter superclasses
would do a `TryExecute` and use that to pass to subclasses in methods like
`DoExecute` and `DoMapField`. Since the dispatcher no longer needs a device
this `TryExecute` is generally unnecessary and always redundant. Thus, it has
been removed.

Because of this, the device argument to `DoExecute` and `DoMapField` has been
removed. This will cause current implementations of filter to change, but it
usually simplifies code. That said, there might be some code that needs to be
wrapped into a `vtkm::cont::ExecObjectBase`.

No changes need to be made to code that uses filters.


## `DeviceAdapterTags` are usable for runtime device selection

VTK-m DeviceAdapterTags now are both a compile time representation of which device to use, and
also the runtime representation of that device. Previously the runtime representation was handled
by `vtkm::cont::DeviceAdapterId`. This was done by making `DeviceAdapterTag`'s' a constexpr type that
inherits from the constexpr `vtkm::cont::DeviceAdapterId` type.

At at ten thousand foot level this change means that in general instead of using `vtkm::cont::DeviceAdapterTraits<DeviceTag>`
you can simply use `DeviceTag`, or an instance of if `DeviceTag runtimeDeviceId;`.

Previously if you wanted to get the runtime representation of a device you would do the following:
```cpp
template<typename DeviceTag>
vtkm::cont::DeviceAdapterId getDeviceId()
{
  using Traits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
  return Traits::GetId();
}
...
vtkm::cont::DeviceAdapterId runtimeId = getDeviceId<DeviceTag>();
```
Now with the updates you could do the following.
```cpp
vtkm::cont::DeviceAdapterId runtimeId = DeviceTag();
```
More importantly this conversion is unnecessary as you can pass instances `DeviceAdapterTags` into methods or functions
that want `vtkm::cont::DeviceAdapterId` as they are that type!


Previously if you wanted to see if a DeviceAdapter was enabled you would the following:
```cpp
using Traits = vtkm::cont::DeviceAdapterTraits<DeviceTag>;
constexpr auto isValid = std::integral_constant<bool, Traits::Valid>();
```
Now you would do:
```cpp
constexpr auto isValid = std::integral_constant<bool, DeviceTag::IsEnabled>();
```

So why did VTK-m make these changes?

That is a good question, and the answer for that is two fold. The VTK-m project is working better support for ArraysHandles that leverage runtime polymorphism (aka virtuals), and the ability to construct `vtkm::worklet::Dispatchers` without specifying
the explicit device they should run on. Both of these designs push more of the VTK-m logic to operate at runtime rather than compile time. This changes are designed to allow for consistent object usage between runtime and compile time instead of having
to convert between compile time and runtime types.



## New geometry classes

There are now some additional structures available both
the control and execution environments for representing
geometric entities (mostly of dimensions 2 and 3).
These new structures are now in `vtkm/Geometry.h` and
demonstrated/tested in `vtkm/testing/TestingGeometry.h`:

+ `Ray<CoordType, Dimension, IsTwoSided>`.
  Instances of this struct represent a semi-infinite line
  segment in a 2-D plane or in a 3-D space, depending on the
  integer dimension specified as a template parameter.
  Its state is the point at the start of the ray (`Origin`)
  plus the ray's `Direction`, a unit-length vector.
  If the third template parameter (IsTwoSided) is true, then
  the ray serves as an infinite line. Otherwise, the ray will
  only report intersections in its positive halfspace.
+ `LineSegment<CoordType, Dimension>`.
  Instances of this struct represent a finite line segment
  in a 2-D plane or in a 3-D space, depending on the integer
  dimension specified as a template parameter.
  Its state is the coordinates of its `Endpoints`.
+ `Plane<CoordType>`.
  Instances of this struct represent a plane in 3-D.
  Its state is the coordinates of a base point (`Origin`) and
  a unit-length normal vector (`Normal`).
+ `Sphere<CoordType, Dimension>`.
  Instances of this struct represent a *d*-dimensional sphere.
  Its state is the coordinates of its center plus a radius.
  It is also aliased with a `using` statement to `Circle<CoordType>`
  for the specific case of 2-D.

These structures provide useful queries and generally
interact with one another.
For instance, it is possible to intersect lines and planes
and compute distances.

For ease of use, there are also several `using` statements
that alias these geometric structures to names that specialize
them for a particular dimension or other template parameter.
As an example, `Ray<CoordType, Dimension, true>` is aliased
to `Line<CoordType, Dimension>` and `Ray<CoordType, 3, true>`
is aliased to `Line3<CoordType>` and `Ray<FloatDefault, 3, true>`
is aliased to `Line3d`.

### Design patterns

If you plan to add a new geometric entity type,
please adopt these conventions:

+ Each geometric entity may be default-constructed.
  The default constructor will initialize the state to some
  valid unit-length entity, usually with some part of
  its state at the origin of the coordinate system.
+ Entities may always be constructed by passing in values
  for their internal state.
  Alternate construction methods are declared as free functions
  such as `make_CircleFrom3Points()`
+ Use template metaprogramming to make methods available
  only when the template dimension gives them semantic meaning.
  For example, a 2-D line segment's perpendicular bisector
  is another line segment, but a 3-D line segment's perpendicular
  line segment is a plane.
  Note how this is accomplished and apply this pattern to
  new geometric entities or new methods on existing entities.
+ Some entities may have invalid state.
  If this is possible, the entity will have an `IsValid()` method.
  For example, a sphere may be invalid because the user or some
  construction technique specified a zero or negative radius.
+ When signed distance is semantically meaningful, provide it
  in favor of or in addition to unsigned distance.
+ Accept a tolerance parameter when appropriate,
  but provide a sensible default value.
  You may want to perform exact arithmetic versions of tests,
  but please provide fast, tolerance-based versions as well.



## Logging support via loguru.

The loguru project has been integrated with VTK-m to provide runtime logging
facilities. A sample of the log output can be found at
https://gitlab.kitware.com/snippets/427.

Logging is enabled by setting the CMake variable VTKm_ENABLE_LOGGING. When
this flag is enabled, any messages logged to the Info, Warn, Error, and
Fatal levels are printed to stderr by default.

Additional logging features are enabled by calling vtkm::cont::InitLogging
in an executable. This will:
- Set human-readable names for the log levels in the output.
- Allow the stderr logging level to be set at runtime by passing a
  '-v [level]' argument to the executable.
- Name the main thread.
- Print a preamble with details of the program's startup (args, etc).
- Install signal handlers to automatically print stacktraces and error
  contexts (linux only) on crashes.

The main logging entry points are the macros VTKM_LOG_S and VTKM_LOG_F,
which using C++ stream and printf syntax, repectively. Other variants exist,
including conditional logging and special-purpose logs for writing specific
events, such as DynamicObject cast results and TryExecute failures.

The logging backend supports the concept of "Scopes". By creating a new
scope with the macros VTKM_LOG_SCOPE or VTKM_LOG_SCOPE_FUNCTION, a new
"logging scope" is opened within the C++ scope the macro is called from. New
messages will be indented in the log until the scope ends, at which point
a message is logged with the elapsed time that the scope was active. Scopes
may be nested to arbitrary depths.

The logging implementation is thread-safe. When working in a multithreaded
environment, each thread may be assigned a human-readable name using
vtkm::cont::SetThreadName. This will appear in the log output so that
per-thread messages can be easily tracked.

By default, only Info, Warn, Error, and Fatal messages are printed to
stderr. This can be changed at runtime by passing the '-v' flag to an
executable that calls vtkm::cont::InitLogging. Alternatively, the
application can explicitly call vtkm::cont::SetStderrLogLevel to change the
verbosity. When specifying a verbosity, all log levels with enum values
less-than-or-equal-to the requested level are printed.
vtkm::cont::LogLevel::Off (or "-v Off") may be used to silence the log
completely.

The helper functions vtkm::cont::GetHumanReadableSize and
vtkm::cont::GetSizeString assist in formating byte sizes to a more readable
format. Similarly, the vtkm::cont::TypeName template functions provide RTTI
based type-name information. When logging is enabled, these use the logging
backend to demangle symbol names on supported platforms.

The more verbose VTK-m log levels are:
- Perf: Logs performance information, using the scopes feature to track
  execution time of filters, worklets, and device algorithms with
  microsecond resolution.
- MemCont / MemExec: These levels log memory allocations in the control and
  execution environments, respectively.
- MemTransfer: This level logs memory transfers between the control and host
  environments.
- Cast: Logs details of dynamic object resolution.

The log may be shared and extended by applications that use VTK-m. There
are two log level ranges left available for applications: User and
UserVerbose. The User levels may be enabled without showing any of the
verbose VTK-m levels, while UserVerbose levels will also enable all VTK-m
levels.



# ArrayHandle

## `ArrayHandle` can now take ownership of a user allocated memory location

Previously memory that was allocated outside of VTK-m was impossible to transfer to
VTK-m as we didn't know how to free it. By extending the ArrayHandle constructors
to support a Storage object that is being moved, we can clearly express that
the ArrayHandle now owns memory it didn't allocate.

Here is an example of how this is done:
```cpp
  T* buffer = new T[100];
  auto user_free_function = [](void* ptr) { delete[] static_cast<T*>(ptr); };

  vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>
      storage(buffer, 100, user_free_function);
  vtkm::cont::ArrayHandle<T> arrayHandle(std::move(storage));
```

## `StorageBasic` now can provide or be provided a delete function

Memory that was allocated outside of VTK-m was impossible to transfer to
VTK-m as we didn't know how to free it. This is now resolved by allowing the
user to specify a free function to be called on release.

Memory that was allocated by VTK-m and Stolen by the user needed the
proper free function. When running on CUDA on hardware that supports
concurrent managed access the free function of the storage could
be `cudaFree`.

To properly steal memory from VTK-m you do the following:
```cpp
  vtkm::cont::ArrayHandle<T> arrayHandle;
  //fill arrayHandle

  //you must get the free function before calling steal array
  auto free_function = arrayHandle.GetDeleteFunction();
  T* ptr = arrayHandle.StealArray();
  //use ptr


  free_function(ptr);
```

## `ArrayHandleTransform` works with ExecObject

Previously, the `ArrayHandleTransform` class only worked with plain old
data (POD) objects as is functors. For simple transforms, this makes sense
since all the data comes from a target `ArrayHandle` that will be sent to
the device through a different path. However, this also requires the
transform to be known at compile time.

However, there are cases where the functor cannot be a POD object and has
to be built for a specific device. There are numerous reasons for this. One
might be that you need some lookup tables. Another might be you want to
support a virtual object, which has to be initialized for a particular
device. The standard way to implement this in VTK-m is to create an
"executive object." This actually means that we create a wrapper around
executive objects that inherits from
`vtkm::cont::ExecutionAndControlObjectBase` that contains a
`PrepareForExecution` method and a `PrepareForControl` method.

As an example, consider the use case of a special `ArrayHandle` that takes
the value in one array and returns the index of that value in another
sorted array. We can do that by creating a functor that finds a value in an
array and returns the index.

```cpp
template <typename ArrayPortalType>
struct FindValueFunctor
{
  ArrayPortalType SortedArrayPortal;

  FindValueFunctor() = default;

  VTKM_CONT FindValueFunctor(const ArrayPortalType& sortedPortal)
    : SortedArrayPortal(sortedPortal)
  { }

  VTKM_EXEC vtkm::Id operator()(const typename PortalType::ValueType& value)
  {
    vtkm::Id leftIndex = 0;
  vtkm::Id rightIndex = this->SortedArrayPortal.GetNubmerOfValues();
  while (leftIndex < rightIndex)
  {
    vtkm::Id middleIndex = (leftIndex + rightIndex) / 2;
    auto middleValue = this->SortedArrayPortal.Get(middleIndex);
    if (middleValue <= value)
    {
      rightIndex = middleValue;
    }
    else
    {
      leftIndex = middleValue + 1;
    }
  }
  return leftIndex;
  }
};
```

Simple enough, except that the type of `ArrayPortalType` depends on what
device the functor runs on (not to mention its memory might need to be
moved to different hardware). We can now solve this problem by creating a
functor objecgt set this up for a device. `ArrayHandle`s also need to be
able to provide portals that run in the control environment, and for that
we need a special version of the functor for the control environment.

```cpp
template <typename ArrayHandleType>
struct FindValueExecutionObject : vtkm::cont::ExecutionAndControlObjectBase
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  ArrayHandleType SortedArray;

  FindValueExecutionObject() = default;

  VTKM_CONT FindValueExecutionObject(const ArrayHandleType& sortedArray)
    : SortedArray(sortedArray)
  { }

  template <typename Device>
  VTKM_CONT
  FindValueFunctor<decltype(std::declval<FunctorType>()(Device()))>
  PrepareForExecution(Device device)
  {
    using FunctorType =
    FindValueFunctor<decltype(std::declval<FunctorType>()(Device()))>

    return FunctorType(this->SortedArray.PrepareForInput(device));
  }

  VTKM_CONT
  FundValueFunctor<typename ArrayHandleType::PortalConstControl>
  PrepareForControl()
  {
    using FunctorType =
    FindValueFunctor<typename ArrayHandleType::PortalConstControl>

  return FunctorType(this->SortedArray.GetPortalConstControl());
  }
}
```

Now you can use this execution object in an `ArrayHandleTransform`. It will
automatically be detected as an execution object and be converted to a
functor in the execution environment.

```cpp
auto transformArray =
  vtkm::cont::make_ArrayHandleTransform(
    inputArray, FindValueExecutionObject<decltype(sortedArray)>(sortedArray));
```


## Add `ArrayHandleView` fancy array

Added a new class named `ArrayHandleView` that allows you to get a subset
of an array. You use the `ArrayHandleView` by giving it a target array, a
starting index, and a length. Here is a simple example of usage:

```cpp
vtkm::cont::ArrayHandle<vtkm::Id> sourceArray;

vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(10), sourceArray);
// sourceArray has [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::Id>>
  viewArray(sourceArray, 3, 5);
// viewArray has [3, 4, 5, 6, 7]
```

There is also a convenience `make_ArraHandleView` function to create view
arrays. The following makes the same view array as before.

```cpp
auto viewArray = vtkm::cont::make_ArrayHandleView(sourceArray, 3, 5);
```

## `ArrayHandleCompositeVector` simplified and made writable.

`ArrayHandleCompositeVector` is now easier to use, as its type has a more
straightforward definition: `ArrayHandleCompositeVector<Array1, Array2, ...>`.
Previously, a helper metaprogramming struct was needed to determine the type
of the array handle.

In addition, the new implementation supports both reading and writing, whereas
the original version was read-only.

Another notable change is that the `ArrayHandleCompositeVector` no longer
supports component extraction from the source arrays. While the previous version
could take a source array with a `vtkm::Vec` `ValueType` and use only a single
component in the output, the new version requires that all input arrays have
the same `ValueType`, which becomes the `ComponentType` of the output
`vtkm::Vec`.

When component extraction is needed, the classes `ArrayHandleSwizzle` and
`ArrayHandleExtractComponent` have been introduced to allow the previous
usecases to continue working efficiently.

## `ArrayHandleExtractComponent` target component is now set at runtime

Rather than embedding the extracted component in a template parameter, the
extract operation is now defined at runtime.

This is easier to use and keeps compile times / sizes / memory requirements
down.

## `ArrayHandleSwizzle` component maps are now set at runtime

Rather than embedding the component map in the template parameters, the swizzle
operation is now defined at runtime using a `vtkm::Vec<vtkm::IdComponent, N>`
that maps the input components to the output components.

This is easier to use and keeps compile times / sizes / memory requirements
down.

# Control Enviornment

## Interfaces for VTK-m spatial search structures added

The objective for this feature was to add a common interface for the VTK-m
spatial search strucutes for ease of use for the users.
VTK-m now distinguishes locators into two types, cell locators and point
locators. Cell locators can be used to query a containing cell for a point,
and point locators can be used to search for other points that are close to the
given point.

All cell locators are now required to inherit from the interface
`vtkm::cont::CellLocator`,  and all point locatos are required to inherit from
the interface `vtkm::cont::PointLocator`

These interfaces describe the necessary features that are required from either
a cell locator, or a point locator and provided an easy way to use them in the
execution environment.

By deriving new search structures from these locator interfaces, it makes it
easier for users to build the underlying structures as well, abstracting away
complicated details. After providing all the required data from a
`vtkm::cont::DataSet` object, the user only need to call the `Update` method
on the object of `vtkm::cont::CellLocator`, or `vtkm::cont::PointLocator`.

For example, building the cell locator which used a Bounding Interval Hiererchy
tree as a search structure, provided in the class
`vtkm::cont::BoundingIntervalHierarchy` which inherits from
`vtkm::cont::CellLocator`, only requires few steps.

```cpp
  // Build a bounding interval hierarchy with 5 splitting planes,
  // and a maximum of 10 cells in the leaf node.
  vtkm::cont::BoundingIntervalHierarchy locator(5, 10);
  // Provide the cell set required by the search structure.
  locator.SetCellSet(cellSet);
  // Provide the coordinate system required by the search structure.
  locator.SetCoordinates(coords);
  // Cell the Update methods to finish building the underlying tree.
  locator.Update();
```
Similarly, users can easily build available point locators as well.

When using an object of `vtkm::cont::CellLocator`, or `vtkm::cont::PointLocator`
in the execution environment, they need to be passed to the worklet as an
`ExecObject` argument. In the execution environment, users will receive a
pointer to an object of type `vtkm::exec::CellLocator`, or
`vtkm::exec::PointLocator` respectively. `vtkm::exec::CellLocator` provides a
method `FindCell` to use in the execution environment to query the containing
cell of a point. `vtkm::exec::PointLocator` provides a method
`FindNearestNeighbor` to query for the nearest point.

As of now,  VTK-m provides only one implementation for each of the given
interfaces. `vtkm::cont::BoundingIntervalHierarchy` which is an implementation
of `vtkm::cont::CellLocator`, and `vtkm::cont::PointLocatorUniformGrid`, which
is an implementation of `vtkm::cont::PointLocator`.



## `vtkm::cont::Algorithm` now can be told which device to use at runtime

The `vtkm::cont::Algorithm` has been extended to support the user specifying
which device to use at runtime previously Algorithm would only use the first
enabled device, requiring users to modify the `vtkm::cont::GlobalRuntimeDeviceTracker`
if they wanted a specific device used.

To select a specific device with vtkm::cont::Algorithm pass the `vtkm::cont::DeviceAdapterId`
as the first parameter.

```cpp
vtkm::cont::ArrayHandle<double> values;

//call with no tag, will run on first enabled device
auto result = vtkm::cont::Algorithm::Reduce(values, 0.0);

//call with an explicit device tag, will only run on serial
vtkm::cont::DeviceAdapterTagSerial serial;
result = vtkm::cont::Algorithm::Reduce(serial, values, 0.0);

//call with an runtime device tag, will only run on serial
vtkm::cont::DeviceAdapterId device = serial;
result = vtkm::cont::Algorithm::Reduce(device, values, 0.0);

```

## Support `ExecArg` behavior in `vtkm::cont::Algorithm` methods

`vtkm::cont::Algorithm` is a wrapper around `DeviceAdapterAlgorithm` that
internally uses `TryExecute`s to select an appropriate device. The
intention is that you can run parallel algorithms (outside of worklets)
without having to specify a particular device.

Most of the arguments given to device adapter algorithms are actually
control-side arguments that get converted to execution objects internally
(usually a `vtkm::cont::ArrayHandle`). However, some of the algorithms,
take an argument that is passed directly to the execution environment, such
as the predicate argument of `Sort`. If the argument is a plain-old-data
(POD) type, which is common enough, then you can just pass the object
straight through. However, if the object has any special elements that have
to be transferred to the execution environment, such as internal arrays,
passing this to the `vtkm::cont::Algorithm` functions becomes problematic.

To cover this use case, all the `vtkm::cont::Algorithm` functions now
support automatically transferring objects that support the `ExecObject`
worklet convention. If any argument to any of the `vtkm::cont::Algorithm`
functions inherits from `vtkm::cont::ExecutionObjectBase`, then the
`PrepareForExecution` method is called with the device the algorithm is
running on, which allows these device-specific objects to be used without
the hassle of creating a `TryExecute`.

## `vtkm::cont::TryExecuteOnDevice` allows for runtime selection of which device to execute on

VTK-m now offers `vtkm::cont::TryExecuteOnDevice` to allow for the user to select
which device to execute a function on at runtime. The original `vtkm::cont::TryExecute`
used the first valid device, which meant users had to modify the runtime state
through the `RuntimeTracker` which was verbose and unwieldy.

Here is an example of how you can execute a function on the device that an array handle was last executed
on:
```cpp

struct ArrayCopyFunctor
{
  template <typename Device, typename InArray, typename OutArray>
  VTKM_CONT bool operator()(Device, const InArray& src, OutArray& dest)
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(src, dest);
    return true;
  }
};

template<typename T, typename InStorage, typename OutStorage>
void SmartCopy(const vtkm::cont::ArrayHandle<T, InStorage>& src, vtkm::cont::ArrayHandle<T, OutStorage>& dest)
{
  bool success = vtkm::cont::TryExecuteOnDevice(devId, ArrayCopyFunctor(), src, dest);
  if (!success)
  {
    vtkm::cont::TryExecute(ArrayCopyFunctor(), src, dest);
  }
}
```

## `vtkm::cont::CellSetExplicit` now caches CellToPoint table when used with Invoke

Issue #268 highlighted an issue where the expensive CellToPoint table
update was not properly cached when a CellSetExplicit was used with a
filter. This has been corrected by ensuring that the metadata
associated with the table survives shallow copying of the CellSet.

New methods are also added to check whether the CellToPoint table
exists, and also to reset it if needed (e.g. for benchmarking):

```
vtkm::cont::CellSetExplicit<> cellSet = ...;
// Check if the CellToPoint table has already been computed:
if (cellSet.HasConnectivity(vtkm::TopologyElementTagCell{},
                            vtkm::TopologyElementTagPoint{}))
{
  // Reset it:
  cellSet.ResetConnectivity(vtkm::TopologyElementTagCell{},
                            vtkm::TopologyElementTagPoint{});
}
```


## `vtkm::cont::RuntimeDeviceInformation` doesn't require a device template

By making RuntimeDeviceInformation class template independent, vtkm is able to detect
device info at runtime with a runtime specified deviceId. In the past it's impossible
because the CRTP pattern does not allow function overloading(compiler would complain
that DeviceAdapterRuntimeDetector does not have Exists() function defined).



## `vtkm::cont::VirtualObjectHandle` can transfer to a device using runtime `DeviceAdapterId` value

Previously `VirtualObjectHandle` required the caller to know a compile time device adapter tag to
transfer data. This was problematic since in parts of VTK-m you would only have the runtime
`vtkm::cont::DeviceAdapterId` value of the desired device. To than transfer the
`VirtualObjectHandle` you would have to call `FindDeviceAdapterTagAndCall`. All this extra
work was unneeded as `VirtualObjectHandle` internally was immediately converting from
a compile time type to a runtime value.


Here is an example of how you can now transfer a `VirtualObjectHandle` to a device using
a runtime value:
```cpp

template<typename BaseType>
const BaseType* moveToDevice(VirtualObjectHandle<BaseType>& handle,
                      vtkm::cont::vtkm::cont::DeviceAdapterId deviceId)
{
  return handle.PrepareForExecution(deviceId);
}
```


## Add new `vtkm::exec` and `vtkm::cont` execution objects

Recent changes to execution objects now have execution objects behave as
factories that create an objec specific for a particular device. Sometimes,
you also need to be able to get an object that behaves properly in the
control environment. For these cases, a sublcass to `vtkm::cont::ExecutionObjectBase`
was created.

This subclass is called `vtkm::cont::ExecutionAndControlObjectBase`. In
addition to the `PrepareForExecution` method required by its superclass,
these objects also need to provide a `PrepareForControl` method to get an
equivalent object that works in the control environment.

See the changelog for `ArrayHandleTransform` works with ExecObject
for an example of using a `vtkm::cont::ExecutionAndControlObjectBase`.

## Added a `ReleaseResources` API to CellSet and its derived classes

We now offer the ability to unload execution memory from `vtkm::cont::CellSet` and its derived
classes(`CellSetExplicit`, `CellSetPermutation` and `CellSetStructured`) using the ReleaseResourcesExecution.


## Added a `ReleaseResourcesExecution` API for Field to unload execution resources

We now offer the ability to unload execution memory from `vtkm::cont::Field` using the ReleaseResourcesExecution method.

## Added a `ReleaseResourcesExecution` API for CoordinateSystem to unload execution resources

We now offer the ability to unload execution memory from `vtkm::cont::ArrayHandleVirtualCoordinates`
and `vtkm::cont::CoordinateSystem` using the ReleaseResourcesExecution method.


## Use the strong typed enums for `vtkm::cont::Field`

By doing so, the compiler would not convert these enums into `int`s
which can cause some unexpected behavior.

## `vtkm::cont::DeviceAdapterId` has becomes a real constexpr type and not an alias to vtkm::UInt8

As part of the ability to support `vtkm::cont::TryExecuteOnDevice` VTK-m has made the
DeviceAdapterId a real constexpr type instead of a vtkm::UInt8.

The benefits of a real type are as follows:

- Easier to add functionality like range verification, which previously had
  to be located in each user of `DeviceAdapterId`

- In ability to have ambiguous arguments. Previously it wasn't perfectly clear
  what a method parameter of `vtkm::UInt8` represented. Was it actually the
  DeviceAdapterId or something else?

- Ability to add subclasses that represent things such as Undefined, Error, or Any.


The implementation of DeviceAdapterId is:
```cpp
struct DeviceAdapterId
{
  constexpr explicit DeviceAdapterId(vtkm::Int8 id)
    : Value(id)
  {
  }

  constexpr bool operator==(DeviceAdapterId other) const { return this->Value == other.Value; }
  constexpr bool operator!=(DeviceAdapterId other) const { return this->Value != other.Value; }
  constexpr bool operator<(DeviceAdapterId other) const { return this->Value < other.Value; }

  constexpr bool IsValueValid() const
  {
    return this->Value > 0 && this->Value < VTKM_MAX_DEVICE_ADAPTER_ID;
  }

  constexpr vtkm::Int8 GetValue() const { return this->Value; }

private:
  vtkm::Int8 Value;
};
```

# Execution Enviornment

## User defined execution objects now usable with runtime selection of device adapter

Changed how Execution objects are created and passed from the `cont` environment to the `exec` environment. Instead we will now fill out a class and call `PrepareForExecution()` and create the execution object for the `exec` environment from this function. This way we do not have to template the class that extends `vtkm::cont::ExecutionObjectBase` on the device.

Example of new execution object:
```cpp
template <typename Device>
struct ExecutionObject
{
  vtkm::Int32 Number;
};

struct TestExecutionObject : public vtkm::cont::ExecutionObjectBase
{
  vtkm::Int32 Number;

  template <typename Device>
  VTKM_CONT ExecutionObject<Device> PrepareForExecution(Device) const
  {
    ExecutionObject<Device> object;
    object.Number = this->Number;
    return object;
  }
};
```

## `Dot` function name changed

The free function `vtkm::dot()` has been renamed to `vtkm::Dot()`
to be consistent with other vtk-m function names.
Aliases are provided for backwards compatibility but will
be removed in the next release.


## Added float version operations for `vtkm::Math Pi()`

`vtkm::Pi<T>` now suports float and double as `T`.

## `vtkm::Math Pi` functions are now constexpr

Now `PI` related functions are evalulated at compile time as constexpr functions.

## `CellDerivativeFor3DCell` has a better version for Vec of Vec fields.

Previously we would compute a 3x3 matrix where each element was a Vec. Using
the jacobain of a single component is sufficient instead of computing it for
each component. This approach saves anywhere from 2 to 3 times the memory space.

## Add a CUDA-safe `vtkm::Swap` method

Added a swap implementation that is safe to call from all backends.

It is not legal to call std functions from CUDA code, and the new
`vtkm::Swap` implements a naive swap when compiled under NVCC while
falling back to a std/ADL swap otherwise.

# Worklets and Filters

## Worklets are now asynchronous on CUDA

Worklets are now fully asynchronous in the CUDA backend. This means that
worklet errors are reported asynchronously. Existing errors are checked for
before invocation of a new worklet and at explicit synchronization points like
`DeviceAdapterAlgorithm<>::Synchronize()`.

An important effect of this change is that functions that are synchronization
points, like `ArrayHandle::GetPortalControl()` and
`ArrayHandle::GetPortalConstControl()`, may now throw exception for errors from
previously executed worklets.

Worklet invocations, synchronization and error reporting happen independtly
on different threads. Therefore, synchronization on one thread does not affect
any other threads.




## Worklets now execute on CUDA using grid stride loops
Previously VTK-m Worklets used what is referred to as a monolithic kernel
pattern for worklet execution. This assumes a single large grid of threads
to process an entire array in a single pass. This resulted in launches that
looked like:

```cpp
template<typename F>
void TaskSingular(F f, vtkm::Id end)
{
  const vtkm::Id index = static_cast<vtkm::Id>(blockDim.x * blockIdx.x + threadIdx.x);
  if (index < end)
  {
    f(index);
  }
}

Schedule1DIndexKernel<TaskSingular><<<totalBlocks, 128, 0, CUDAStreamPerThread>>>(
       functor, numInstances);
```

This was problematic as it had the drawbacks of:
- Not being able to reuse any infrastructure between kernel executions.
- Harder to tune performance based on the current hardware.

The solution was to move to a grid stride loop strategy with a block size
based off the number of SM's on the executing GPU. The result is something
that looks like:

```cpp
template<typename F>
void TaskStrided(F f, vtkm::Id end)
{
  const vtkm::Id start = blockIdx.x * blockDim.x + threadIdx.x;
  const vtkm::Id inc = blockDim.x * gridDim.x;
  for (vtkm::Id index = start; index < end; index += inc)
  {
    f(index);
  }
}
Schedule1DIndexKernel<TaskStrided><<<32*numSMs, 128, 0, CUDAStreamPerThread>>>(
       functor, numInstances);
```

 With a loop stride equal to grid size we maintain the optimal memory
 coalescing patterns as we had with the monolithic version. These changes
 also allow VTK-m to optimize TaskStrided so that it can reuse infrastructure
 between iterations.


## Scatter class moved to dispatcher

Scatter classes are special objects that are associated with a worklet to
adjust the standard 1:1 mapping of input to output in the worklet execution
to some other mapping with multiple outputs to a single input or skipping
over input values. A classic use case is the Marching Cubes algorithm where
cube cases will have different numbers of output. A scatter object allows
you to specify for each output polygon which source cube it comes from.

Scatter objects have been in VTK-m for some time now (since before the 1.0
release). The way they used to work is that the worklet completely managed
the scatter object. It would declare the `ScatterType`, keep a copy as part
of its state, and provide a `GetScatter` method so that the dispatcher
could use it for scheduling.

The problem with this approach is that it put control-environment-specific
state into the worklet. The scatter object would be pushed into the
execution environment (like a CUDA device) like the rest of the worklet
where it could not be used. It also meant that worklets that defined their
own scatter had to declare a bunch more code to manage the scatter.

This behavior has been changed so that the dispatcher object manages the
scatter object. The worklet still declares the type of scatter by declaring
a `ScatterType` (defaulting to `ScatterUniform` for 1:1 mapping),
but its responsibility ends there. When the dispatcher is constructed, it
must be given a scatter object that matches the `ScatterType` of the
associated worklet. (If `ScatterType` has a default constructor, then one
can be created automatically.) A worklet may declare a static `MakeScatter`
method for convenience, but this is not necessary.

As an example, a worklet may declare a custom scatter like this.

```cpp
  class Generate : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<Vec3> inPoints,
                                  FieldOut<Vec3> outPoints);
    typedef void ExecutionSignature(_1, _2);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CountArrayType, typename DeviceAdapterTag>
    VTKM_CONT
    static ScatterType MakeScatter(const CountArrayType &countArray,
                                   DeviceAdapterTag)
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
      return ScatterType(countArray, DeviceAdapterTag());
    }
```

Note that the `ScatterCounting` needs to be created with the appropriate
indexing arrays to make the scatter behave as the worklet expects, so the
worklet provides a helpful `MakeScatter` method to make it more clear how
to construct the scatter.

This worklet can be invoked as follows.

```cpp
    auto generateScatter =
        ClipPoints::Generate::MakeScatter(countArray, DeviceAdapterTag());
    vtkm::worklet::DispatcherMapField<ClipPoints::Generate, DeviceAdapterTag>
        dispatcherGenerate(generateScatter);
    dispatcherGenerate.Invoke(pointArray, clippedPointsArray);
```

Because the `ScatterCounting` class does not have a default constructor,
you would get a compiler error if you failed to provide one to the
dispatcher's constructor. The compiler error will probably not be too
helpful the the user, but there is a detailed comment in the dispatcher's
code where the compiler error will occur describing what the issue is.


## VTK-m now supports dispatcher parameters being pointers

Previously it was only possible to pass values to a dispatcher when
you wanted to invoke a VTK-m worklet. This caused problems when it came
to designing new types that used inheritance as the types couldn't be
past as the base type to the dispatcher. To fix this issue we now
support invoking worklets with pointers as seen below.

```cpp
  vtkm::cont::ArrayHandle<T> input;
  //fill input

  vtkm::cont::ArrayHandle<T> output;
  vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;

  dispatcher(&input, output);
  dispatcher(input, &output);
  dispatcher(&input, &output);
```



## Histogram filter supports custom types

By passing TypeList and StorageList type into FieldRangeGlobalCompute,
upstream users(VTK) can pass custom types into the histogram filter.


## Cell measure functions, worklet, and filter

VTK-m now provides free functions, a worklet, and a filter for computing
the integral measure of a cell (i.e., its arc length, area, or volume).

The free functions are located in `vtkm/exec/CellMeasure.h` and share the
same signature:

```c++
  template<typename OutType, typename PointVecType>
  OutType CellMeasure(
    const vtkm::IdComponent& numPts,
    const PointCoordVecType& pts,
    CellShapeTag,
    const vtkm::exec::FunctorBase& worklet);
```

The number of points argument is provided for cell-types such as lines,
which allow an arbitrary number of points per cell.
See the worklet for examples of their use.

The worklet is named `vtkm::worklet::CellMeasure` and takes a template
parameter that is a tag list of measures to include.
Cells that are not selected by the tag list return a measure of 0.
Some convenient tag lists are predefined for you:

+ `vtkm::ArcLength` will only compute the measure of cells with a 1-dimensional parameter-space.
+ `vtkm::Area` will only compute the measure of cells with a 2-dimensional parameter-space.
+ `vtkm::Volume` will only compute the measure of cells with a 3-dimensional parameter-space.
+ `vtkm::AllMeasures` will compute all of the above.

The filter version, named `vtkm::filter::CellMeasures` – plural since
it produces a cell-centered array of measures — takes the same template
parameter and tag lists as the worklet.
By default, the output array of measure values is named "measure" but
the filter accepts other names via the `SetCellMeasureName()` method.

The only cell type that is not supported is the polygon;
you must triangulate polygons before running this filter.
See the unit tests for examples of how to use the worklet and filter.

The cell measures are all signed: negative measures indicate that the cell is inverted.
Simplicial cells (points, lines, triangles, tetrahedra) cannot not be inverted
by definition and thus always return values above or equal to 0.0.
Negative values indicate either the order in which vertices appear in its connectivity
array is improper or the relative locations of the vertices in world coordinates
result in a cell with a negative Jacobian somewhere in its interior.
Finally, note that cell measures may return invalid (NaN) or infinite (Inf, -Inf)
values if the cell is poorly defined, e.g., has coincident vertices
or a parametric dimension larger than the space spanned by its world-coordinate
vertices.

The verdict mesh quality library was used as the source of the methods
for approximating the cell measures.



## Add a `WaveletGenerator` worklet (e.g. vtkRTAnalyticSource)

Add a VTK-m implementation of VTK's `vtkRTAnalyticSource`, or "Wavelet" source
as it is known in ParaView. This is a customizable dataset with properties
that make it useful for testing and benchmarking various algorithms.

## Add a filter to support Lagrangian analysis capabilities

Lagrangian analysis operates in two phases - phase one involes the extraction of flow field information. Phase two involves calculating new particle trajectories using the saved information.

The lagrangian filter can be used to extract flow field information given a time-varying vector fields. The extracted information is in the form of particle trajectories.

The filter operates by first being set up with some information regarding step size, the interval at which information should be saved (write frequency), the number of seeds to be placed in the domain (specified as a reduction factor along each axis of the original dimensions).
The step size should be equivalent to the time between vector field data input.
The write frequency corresponds to the number of cycles between saves.

Filter execution is called for each cycle of the simulation data. Each filter execution call requires a velocity field to advect particles forward.

The extracted particle trajectories - referred to as basis flows exist in the domain for the specified interval (write frequency). Particles are then reset along a uniform grid and new particle trajectories are calculated.

An example of using the Lagrangian filter is at vtk-m/examples/lagrangian
The basis flows are saved into a folder named output which needs to be created in the directory in which the program is being executed.

The basis flows can be interpolated using barycentric coordinate interpolation or a form of linear interpolation to calculate new particle trajectories post hoc.

An example of using basis flows generated by the Lagrangian filter is at vtk-m/examples/posthocinterpolation. The folder contains a script which specifies parameters which need to be provided to use the example.

## Add a warp vector worklet and filter

This commit adds a worklet that modifies point coordinates by moving points
along point normals by the scalar amount. It's a simplified version of the
vtkWarpScalar in VTK. Additionally the filter doesn't modify the point coordinates,
but creates a new point coordinates that have been warped.
Useful for showing flow profiles or mechanical deformation.


## Add a warp scalar worklet and filter

This commit adds a worklet as well as a filter that modify point coordinates by moving points
along point normals by the scalar amount times the scalar factor.
It's a simplified version of the vtkWarpScalar class in VTK. Additionally the filter doesn't
modify the point coordinates, but creates a new point coordinates that have been warped.



## Add a split sharp edge filter

It's a filter that splits sharp manifold edges where the feature angle
between the adjacent surfaces are larger than the threshold value.
When an edge is split, it would add a new point to the coordinates
and update the connectivity of an adjacent surface.
Ex. There are two adjacent triangles(0,1,2) and (2,1,3). Edge (1,2) needs
to be split. Two new points 4(duplication of point 1) an 5(duplication of point 2)
would be added and the later triangle's connectivity would be changed
to (5,4,3).
By default, all old point's fields would be copied to the new point.
Use with caution.

## Time-varying "oscillator" filter and example

The oscillator is a simple analytical source of time-varying data.
It provides a function value at each point of a uniform grid that
is computed as a sum of Gaussian kernels — each with a specified
position, amplitude, frequency, and phase.

The example (in `examples/oscillator`) generates volumetric Cinema
datasets that can be viewed in a web browser with [ArcticViewer][].

[ArcticViewer]: https://kitware.github.io/arctic-viewer/


# Build

## Build System Redesign and new minimum CMake

VTK-m CMake buildsystem was redesigned to be more declarative for consumers.
This was done by moving away from the previous component design and instead
to explicit targets. Additionally VTK-m now uses the native CUDA support
introduced in CMake 3.8 and has the following minimum CMake versions:
 - Visual Studio Generator requires CMake 3.11+
 - CUDA support requires CMake 3.9+
 - OpenMP support requires CMake 3.9+
 - Otherwise CMake 3.3+ is supported

When VTK-m is found find_package it defines the following targets:
  - `vtkm_cont`
    - contains all common core functionality
    - always exists

  - `vtkm_rendering`
    - contains all the rendering code
    - exists only when rendering is enabled
    - rendering also provides a `vtkm_find_gl` function
      - allows you to find the GL (EGL,MESA,Hardware), GLUT, and GLEW
        versions that VTK-m was built with.

VTK-m also provides targets that represent what device adapters it
was built to support. The pattern for these targets are `vtkm::<device>`.
Currently we don't provide a target for the serial device.

  - `vtkm::tbb`
    - Target that contains tbb related link information
       implicitly linked to by `vtkm_cont` if tbb was enabled

  - `vtkm::openmp`
    - Target that contains openmp related link information
       implicitly linked to by `vtkm_cont` if openmp was enabled

  - `vtkm::CUDA`
    - Target that contains CUDA related link information
       implicitly linked to by `vtkm_cont` if CUDA was enabled

VTK-m can be built with specific CPU architecture vectorization/optimization flags.
Consumers of the project can find these flags by looking at the `vtkm_vectorization_flags`
target.

So a project that wants to build an executable that uses vtk-m would look like:

```cmake

cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
project(HellowWorld CXX)

#Find the VTK-m package.
#Will automatically enable the CUDA language if needed ( and bump CMake minimum )

find_package(VTKm REQUIRED)

add_executable(HelloWorld HelloWorld.cxx)
target_link_libraries(HelloWorld PRIVATE vtkm_cont)

if(TARGET vtkm::CUDA)
  set_source_files_properties(HelloWorld.cxx PROPERTIES LANGUAGE CUDA)
endif()

```


## Add `none` option to `VTKm_CUDA_Architecture`

A new VTKm_CUDA_Architecture option called `none` has been added. This will
disable all VTK-m generated CUDA architecture flags, allowing the user to
specify their own custom flags.

Useful when VTK-m is used as a library in another project and the project wants
to use its own architecture flags.

## Add a new cmake option: `VTKm_ENABLE_DEVELOPER_FLAGS`

The new cmake option VTKm_ENABLE_DEVELOPER_FLAGS can be used to enable/disable
warnings in VTK-m. It is useful to disable VTK-m's warning flags when VTK-m is
directly embedded by a project as sub project (add_subdirectory), and the
warnings are too strict for the project. This does not apply when using an
installed version of VTK-m.

For example, this flag is disabled in VTK.

This flag is enabled by default.

## Add a new cmake option: `VTKm_INSTALL_ONLY_LIBRARIES`

The new cmake option VTKm_INSTALL_ONLY_LIBRARIES when enabled will cause
VTK-m to only install libraries. This is useful for projects that are
producing an application and don't want to ship headers or CMake infrastructure.

For example, this flag is enabled by ParaView for releases.

This flag is disabled by default.

# Other

## Control CUDA managed memory with an environment variable

By setting the environment variable "VTKM_MANAGEDMEMO_DISABLED" to be 1,
users are able to disable CUDA managed memory even though the hardware is capable
of doing so.

## Add support for deferred freeing of CUDA memory

A new function, `void CudaAllocator::FreeDeferred(void* ptr, std::size_t numBytes)` has
been added that can be used to defer the freeing of CUDA memory to a later point.
This is useful because `cudaFree` causes a global sync across all CUDA streams. This function
internally maintains a pool of to-be-freed pointers that are freed together when a
size threshold is reached. This way a number of global syncs are collected together at
one point.



## Allow variable arguments to `VTKM_TEST_ASSERT`

The `VTKM_TEST_ASSERT` macro is a very useful tool for performing checks
in tests. However, it is rather annoying to have to always specify a
message for the assert. Often the failure is self evident from the
condition (which is already printed out), and specifying a message is
both repetative and annoying.

Also, it is often equally annoying to print out additional information
in the case of an assertion failure. In that case, you have to either
attach a debugger or add a printf, see the problem, and remove the
printf.

This change solves both of these problems. `VTKM_TEST_ASSERT` now takes a
condition and a variable number of message arguments. If no message
arguments are given, then a default message (along with the condition)
are output. If multiple message arguments are given, they are appended
together in the result. The messages do not have to be strings. Any
object that can be sent to a stream will be printed correctly. This
allows you to print out the values that caused the issue.

So the old behavior of `VTKM_TEST_ASSERT` still works. So you can have a
statement like

```cpp
VTKM_TEST_ASSERT(array.GetNumberOfValues() != 0, "Array is empty");
```

As before, if this assertion failed, you would get the following error
message.

```
Array is empty (array.GetNumberOfValues() != 0)
```

However, in the statement above, you may feel that it is self evident that
`array.GetNumberOfValues() == 0` means the array is empty and you have to
type this into your test, like, 20 times. You can save yourself some work
by dropping the message.

```cpp
VTKM_TEST_ASSERT(array.GetNumberOfValues() != 0);
```

In this case if the assertion fails, you will get a message like this.

```
Test assertion failed (array.GetNumberOfValues() != 0)
```

But perhaps you have the opposite problem. Perhaps you need to output more
information. Let's say that you expected a particular operation to half the
length of an array. If the operation fails, it could be helpful to know how
big the array actually is. You can now actually output that on failure by
adding more message arguments.

```cpp
VTKM_TEST_ARRAY(outarray.GetNumberOfValues() == inarrayGetNumberOfValues()/2,
                "Expected array size ",
        inarrayGetNumberOfValues()/2,
        " but got ",
        outarray.GetNumberOfValues());
```

In this case, if the test failed, you might get an error like this.

```
Expected array size 5 but got 6 (outarray.GetNumberOfValues() == inarrayGetNumberOfValues()/2)
```



## Support `constexpr` and variadic constructor for Vec

Add variadic constructors to the `vtkm::Vec` classes. The main advantage of this addition
is that it makes it much easier to initialize `Vec`s of arbitrary length.

Meanwhile, `Vec` classes constructed with values listed in their parameters up to size 4
are constructed as constant expressions at compile time to reduce runtime overhead.
Sizes greater than 4 are not yet supported to be constructed at compile time via initializer lists
since in C++11 constexpr does not allow for loops. Only on Windows platform with a compiler older
than Visual Studio 2017 version 15.0, users are allowed to use initializer lists to construct a
vec with size > 4.

`vtkm::make_Vec` would always construct `Vec` at compile time if possible.

```cpp
vtkm::Vec<vtkm::Float64, 3> vec1{1.1, 2.2, 3.3};  // New better initializer since
                                                  // it does not allow type narrowing

vtkm::Vec<vtkm::Float64, 3> vec2 = {1.1, 2.2, 3.3}; // Nice syntax also supported by
                                                    // initializer lists.

vtkm::Vec<vtkm::Float64, 3> vec3 = vtkm::make_Vec(1.1, 2.2, 3.3); // Old style that still works.

vtkm::Vec<vtkm::Float64, 3> vec3(1.1, 2.2, 3.3); // Old style that still works but
                                                 // should be deprecated. Reason listed below.
```

Nested initializer lists work to initialize `Vec` of `Vec`s. If the size is no more than 4,
it's always constructed at compile time if possible.

```cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec{ {1.1, 2.2}, {3.3, 4.4}, {5.5, 6.6} };
                                                           //Constructed at compile time
```

One drawback about the `std::initializer_list` implementation is that it constructs larger
`Vec`(size>4) of scalars or `vec`s at run time.

```cpp
vtkm::Vec<vtkm::Float64, 5> vec1{1.1, 2.2, 3.3, 4.4, 5.5}; // Constructed at run time.

vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 5> vec2{{1.1, 1.1},{2.2, 2.2},{3.3, 3.3},
                                      {4.4, 4.4}, {5.5, 5.5}}; // Constructed at run time.
```

Parenthesis constructor would report an error if the size is larger than 4 when being used
to construct a `Vec` of `Vec`s. If it's being used to construct a `Vec` of scalars then it's
fine.

```cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 5> vec2({1.1, 1.1},{2.2, 2.2},{3.3, 3.3},
                                      {4.4, 4.4}, {5.5, 5.5}); // ERROR! This type of
                                                               // constructor not implemented!

vtkm::Vec<vtkm::Float64, 5> vec1(1.1, 2.2, 3.3, 4.4, 5.5); // Constructed at compile time.
```

If a `vtkm::Vec` is initialized with a list of size one, then that one value is
replicated for all components.

```cpp
vtkm::Vec<vtkm::Float64, 3> vec{1.1};  // vec gets [ 1.1, 1.1, 1.1 ]
```

This "scalar" initialization also works for `Vec` of `Vec`s.

```cpp
vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec1{ { 1.1, 2.2 } };
// vec1 is [[1.1, 2.2], [1.1, 2.2], [1.1, 2.2]]

vtkm::Vec<vtkm::Vec<vtkm::Float64, 2>, 3> vec2{ { 3.3}, { 4.4 }, { 5.5 } };
// vec2 is [[3.3, 3.3], [4.4, 4.4], [5.5, 5.5]]
```

`vtkm::make_Vec` is also updated to support an arbitrary number initial values which are
constructed at compile time.

```cpp
// Creates a vtkm::Vec<vtkm::Float64, 5>
auto vec = vtkm::make_Vec(1.1, 2.2, 3.3, 4.4, 5.5);
```

This is super convenient when dealing with variadic function arguments.

```cpp
template <typename... Ts>
void ExampleVariadicFunction(const Ts&... params)
{
  auto vec = vtkm::make_Vec(params...);
```

Of course, this assumes that the type of all the parameters is the same. If not, you
could run into compiler trouble.

`vtkm::make_Vec` does not accept an `std::initializer_list`,

```cpp
// Creates a vtkm::Vec<vtkm::Float64, 3>
auto vec1 = vtkm::make_Vec<3>({1.1, 2.2, 3.3}); // ERROR

// Creates exactly the same thing but compiles
auto vec1 = vtkm::make_Vec<3>(1.1, 2.2, 3.3);
```

A limitation of the initializer list constructor is that the compiler has no way to
check the length of the list or force it to a particular length. Thus it is entirely
possible to construct a `Vec` with the wrong number of arguments. Or, more to the
point, the compiler will let you do it, but there is an assert in the constructor to
correct for that. (Of course, asserts are not compiled in release builds.)

```cpp
// This will compile, but it's results are undefined when it is run.
// In debug builds, it will fail an assert.
vtkm::Vec<vtkm::Float64, 3> vec{1.1, 1.2};
```


## `vtkm::Vec< vtkm::Vec<T> >` can't be constructed from `vtkm::Vec<U>`


When you have a Vec<Vec<float,3>> it was possible to incorrectly initialize
it with the contents of a Vec<double,3>. An example of this is:
```cpp
using Vec3d = vtkm::Vec<double, 3>;
using Vec3f = vtkm::Vec<float, 3>;
using Vec3x3f = vtkm::Vec<Vec3f, 3>;

Vec3d x(0.0, 1.0, 2.0);
Vec3x3f b(x); // becomes [[0,0,0],[1,1,1],[2,2,2]]
Vec3x3f c(x, x, x); // becomes [[0,1,2],[0,1,2],[0,1,2]]
Vec3x3f d(Vec3f(0.0f,1.0f,2.0f)) //becomes [[0,0,0],[1,1,1],[2,2,2]]
```

So the solution we have chosen is to disallow the construction of objects such
as b. This still allows the free implicit cast to go from double to float.


## Use `std::call_once` to construct singeltons

By using call_once from C++11, we can simplify the logic in code where we are querying same value variables from multiple threads.


## Use `thread_local` in `vtkm::cont::GetGlobalRuntimeDeviceTracker` function if possible

It will reduce the cost of getting the thread runtime device tracker,
and will have a better runtime overhead if user constructs a lot of
short lived threads that use VTK-m.



## Replace `std::random_shuffle` with `std::shuffle`

std::random_shuffle is deprecated in C++14 because it's using std::rand
which uses a non uniform distribution and the underlying algorithm is
unspecified. Using std::shuffle can provide a reliable result in a 64 bit version.



