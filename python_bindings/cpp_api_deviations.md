## Python API Deviations from C++

This file tracks intentional differences between the Python bindings and the C++
API. Keep this list current as bindings move closer to the C++ surface.

### C++-Matching Return Types To Preserve

These bindings are intentionally close to the C++ API and should not be
"simplified" to broader Python convenience types:

- `Field.GetData()` returns `UnknownArrayHandle`.
- `CoordinateSystem.GetData()` returns
  `UncertainArrayHandle`, currently bound to the broad C++ instantiation
  `viskores::cont::UncertainArrayHandle<VISKORES_DEFAULT_TYPE_LIST,
  VISKORES_DEFAULT_STORAGE_LIST>`.

Revisit the Python lifetime and mutation semantics for `Field.GetData()` before
finalizing the API: the desired behavior is C++-like access to the field's
stored array handle without surprising dangling-reference or stale-handle cases.

Also revisit the broader Python type model for `UnknownArrayHandle` and
`UncertainArrayHandle`; see `python_api_plan.md`. The current experiment uses
the default Viskores type and storage lists for the public uncertain-array
binding so Python can cover the Viskores-supported array set without exposing
template-specific class names. Binding code may still need to narrow this broad
handle to subset-specific `UncertainArrayHandle<...>` instantiations when
calling C++ APIs that require a smaller candidate list.

The same model is used for cell sets: public `UncertainCellSet` is bound to
`viskores::cont::UncertainCellSet<VISKORES_DEFAULT_CELL_SET_LIST>`, while
subset-specific lists such as `VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED` should
be constructed at the binding boundary when a C++ API requires them.

### Python Convenience Namespace

`viskores.python_convenience` contains helpers that do not correspond directly
to C++ names:

- `create_uniform_dataset`
- `partition_uniform_dataset`
- `array_from_numpy`
- `asnumpy`
- `field_names`
- `cell_set_type_name`

These helpers are intentionally isolated from `viskores` and `viskores.cont`.

### NumPy Data Access

The bindings add NumPy accessors for Python interoperability:

- `UnknownArrayHandle.AsNumPy`
- concrete array-family `AsNumPy` methods
- `ArrayHandleGroupVecVariable*.AsList`
- rendering color/depth buffer accessors that return NumPy arrays
- scalar-topology helper accessors that return NumPy arrays

These are Python accessors for array data, not replacements for C++ object
returns such as `Field.GetData()` or `CoordinateSystem.GetData()`.

### Python Argument Conversion

Several methods accept Python arrays, lists, or tuples and convert them to C++
types such as `UnknownArrayHandle`, `ArrayHandle`, `Vec`, `Id2`, or `Id3`.
Examples include:

- `CoordinateSystem` constructors and `CoordinateSystem.SetData`
- `make_Field`, `make_FieldPoint`, and `make_FieldCell`
- `DataSet.AddPointField`, `DataSet.AddCellField`,
  `DataSet.SetGhostCellField`, and `DataSet.AddCoordinateSystem`
- `DataSetBuilder*` construction helpers
- `Probe.SetGeometry`, which accepts a `DataSet` or NumPy-compatible point
  array
- `TransferToOpenGL`, which accepts a dataset field/coordinate system or a
  NumPy-compatible array
- camera vector setters and rendering color/bounds/range arguments
- generated method arguments with manifest converters such as `Range`,
  `Bounds`, `RangeId3`, `StringList`, `Dimensions`, `Float64List`, `IdList`,
  `UnknownArray`, `Id2`, `Id3`, and `Vec3f`

Tuple conversions should require the tuple length to match the C++ vector type.
Dimension arguments are the explicit exception: some dataset/source builders
accept 1-, 2-, or 3-tuples because tuple length selects the dimensional rank.

For this iteration, `viskores::Vec`, `viskores::Id2`, and `viskores::Id3` are
intentionally represented as exact-length Python tuples rather than Python
wrapper classes. They are tuple-like value types, so this keeps the bindings
simple without losing meaningful object behavior. Revisit this later only if
real wrapper classes become necessary for API fidelity or mutability.

### Python Return Shapes

Some return values are converted to Python container forms rather than exposing
the exact C++ return object:

- generated `viskores::Id2`, `viskores::Id3`, `viskores::Vec3f`, and
  `viskores::Vec3f_32` returns become Python tuples
- `CoordinateSystem.GetRange()` returns a Python tuple of three
  `viskores.Range` objects for the C++ `viskores::Vec<viskores::Range, 3>`
  return
- `Camera.GetViewRange2D()` returns a 4-tuple
- `UnknownCellSet.GetCellPointIds()` returns a Python sequence rather than
  requiring a caller-allocated buffer
- `ScalarRenderer.Result` exposes Python read-only properties with NumPy/list
  and dict conversions
- contour-tree helper methods expose selected internal arrays and derived
  results as NumPy arrays or Python dictionaries

### Ownership And Lifetime Wrappers

Some bindings use Python ownership wrappers to make returned objects usable from
Python:

- rendering constructors and setters accept shared-owned Python `DataSet`,
  `PartitionedDataSet`, `Actor`, `Scene`, and `Camera` objects and unwrap them
  before calling C++
- `PartitionedDataSet.GetPartition()` returns a reference to the contained
  `DataSet` with nanobind `reference_internal` lifetime handling
- `Scene.GetActor()` returns a Python-owned `Actor` copy
- `BufferState` is exposed through a Python holder class around
  `viskores::interop::BufferState`

Several manual classes remain manual primarily because of these ownership and
argument-conversion wrappers. Revisit them after the array-handle design and
after deciding which Python ownership wrappers should remain.

### Python Object Protocol

Some classes add Python-specific behavior such as `__repr__`, `__len__`,
`__getitem__`, Python comparison operators, tuple/list return forms, or Python
exception messages. These should remain thin wrappers around the same C++ state
and methods.

### Unbound Declared Constructors

`viskores::cont::ColorTable` declares range-based constructors that are not
linked/exported in the current build, so the Python binding omits those
constructors. Use the generated constructor plus `RescaleToRange` instead.

### Template Instantiations With Public Python Names

Some templated C++ classes use concrete Python names such as:

- `ArrayHandleSOAVec3f`
- `ArrayHandleRecombineVecFloatDefault`
- `ArrayHandleGroupVecVariableFloatDefault`
- `ArrayHandleGroupVecVariableId`

`UncertainArrayHandle` is different: the public Python name is currently bound
to the broad default-list C++ instantiation rather than to a template-specific
name. Additional subset-specific `UncertainArrayHandle<...>` instantiations
should remain internal implementation details unless the Python API explicitly
needs to expose the type-list distinction.

### Excluded Implementation Classes

Some C++ implementation classes are deliberately not exposed as Python classes:

- abstract image reader/writer bases, except `ImageWriterBase` is exposed for
  the nested `PixelDepth` enum and inherited writer API
- VTK reader variants covered by `VTKDataSetReader`
- mesh-quality metric subclasses covered by `MeshQuality`
- contour implementation bases and variants covered by `Contour`
