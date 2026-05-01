# Python Binding API Plan

This note tracks open Python API design items that should be resolved before
the bindings are treated as stable.

## Array Handle Type Model

Rethink the Python binding approach for `ArrayHandle`, `UnknownArrayHandle`,
and `UncertainArrayHandle`.

The current experiment exposes `UnknownArrayHandle` as the non-template C++
runtime-polymorphic holder and exposes public `UncertainArrayHandle` as:

```cpp
viskores::cont::UncertainArrayHandle<
  VISKORES_DEFAULT_TYPE_LIST,
  VISKORES_DEFAULT_STORAGE_LIST>
```

This follows the C++ documentation suggestion for cases where a finite set of
types is required but no smaller subset is known. It also keeps Python from
exposing long template-specific class names as public API. The same model is now
used for public `UncertainCellSet`, which is bound as
`viskores::cont::UncertainCellSet<VISKORES_DEFAULT_CELL_SET_LIST>`.

The main expected complication is subset-specific C++ APIs. If a C++ method
expects a narrower uncertain holder, the Python binding should accept the broad
public `UncertainArrayHandle` / `UncertainCellSet` or the corresponding
`Unknown*` holder and explicitly construct the narrower C++ type at the binding
boundary.

The design pass should decide how Python should represent:

- concrete `ArrayHandle<T, Storage>` template instantiations when they need to
  be visible as real Python objects
- the runtime-polymorphic array object held by `UnknownArrayHandle`
- the template-constrained possible type/storage lists of
  `UncertainArrayHandle<ValueTypeList, StorageTypeList>`
- additional future `UncertainArrayHandle<...>` instantiations without leaking
  long template-specific implementation names into the public Python API
- NumPy accessors and other Python-only conveniences without obscuring the C++
  object model

The goal is to stay close to the C++ API while still giving Python users a
clear way to inspect, pass, and convert array handles.

For many Python entry points, NumPy arrays may be the right storage-facing
object instead of exposing concrete `ArrayHandle<T, Storage>` wrappers. The
current `NumPyVec3fArray` manifest converter is a binding-boundary adapter for
APIs such as `Probe.SetGeometry` that accept a concrete
`ArrayHandle<viskores::Vec3f>` in C++; the Python-facing contract is a
NumPy-compatible Nx3 coordinate array. It should not be treated as the final
public model for the full `ArrayHandle<T, Storage>` template family.

## Remaining C++ API Alignment

After the array-handle model is settled, review the remaining Python/C++
differences documented in `cpp_api_deviations.md`.

Current low-risk cleanup targets are:

- keep `viskores::Vec`, `viskores::Id2`, and `viskores::Id3` as exact-length
  Python tuples for this iteration because they are tuple-like value types;
  revisit this later only if real wrapper classes become useful
- keep `viskores::io::ImageWriterBase` exposed manually for C++ API fidelity
  because it is the abstract base of the concrete image writers and owns
  `PixelDepth`; revisit later whether abstract bases should be generated or
  documented as intentionally manual
- revisit manual bindings whose only remaining blockers are tuple conversion,
  range/bounds conversion, or Python ownership wrappers

`Actor.GetScalarRange()` has been aligned with C++ and now returns
`viskores.Range`.

`CoordinateSystem.GetBounds()` and generated `GetBounds()` methods have been
aligned with C++ and now return `viskores.Bounds`.

`CoordinateSystem` has been converted from a manual binding to a generated
manifest entry. Its NumPy-facing constructor and `SetData` use the reusable
`UnknownArray` converter, `GetData` uses the broad public `UncertainArrayHandle`
return converter, and `GetRange()` now returns a tuple of `viskores.Range`
objects to match the C++ `viskores::Vec<viskores::Range, 3>` shape more closely.

`DataSet` has been converted from a manual binding to a generated manifest
entry. The previous `nb::args` dispatchers for `SetGhostCellField`,
`GetCoordinateSystem`, and `AddCoordinateSystem` are now explicit generated
overloads, with `UnknownArray` marking the overloads that accept Python
array-like input.

`PartitionedDataSet` has been converted from a manual binding to a generated
manifest entry. `AppendPartition` now accepts a `DataSet` directly, and
`GetPartition` returns a reference to the contained `DataSet` with nanobind
`reference_internal` lifetime handling instead of returning a Python-owned copy.

`DataSetBuilderExplicitIterative` has been converted from a manual binding to a
generated manifest entry. Its point and connectivity parsing now use reusable
manifest converters while preserving the existing C++ method names.

`DataSetBuilderUniform` has been converted from a manual binding to a generated
manifest entry. Its static `Create` method uses the shared `Dimensions`,
`Vec3fDefaultZero`, and `Vec3fDefaultOne` converters.

`DataSet.FieldNames()` and `DataSet.CellSetTypeName()` have been removed from
the C++-named `DataSet` wrapper. Their Python-only replacements live in
`viskores.python_convenience.field_names` and
`viskores.python_convenience.cell_set_type_name`.

`Probe` has been converted from a manual binding to a generated manifest entry.
Its `SetGeometry` overloads demonstrate the intended pattern for explicit
argument conversion: one overload accepts `viskores::cont::DataSet` directly and
one overload uses the reusable `NumPyVec3fArray` converter for point arrays.

`ContourTreeUniformDistributed` has also been converted to a generated manifest
entry. Its `SetBlockIndices` method uses `NumPyId3Array` for the C++ local block
index array while exposing a NumPy-facing Python argument.
