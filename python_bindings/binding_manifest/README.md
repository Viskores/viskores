<!--
=============================================================================
  The contents of this file are covered by the Viskores license. See
  LICENSE.txt for details.

  By contributing to this file, all contributors agree to the Developer
  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
=============================================================================
-->

# Python Binding Manifest

The YAML files in this directory describe the public Python binding surface.
Generated entries drive the binding generator, while `manual` and `excluded`
entries act as a coverage ledger for hand-written or intentionally omitted
classes. The manifest keeps the generated Python API explicit without losing
track of the classes that still require custom nanobind code.

Each top-level list item describes one C++ class. Classes without a `binding`
field are generated. Classes with `binding: manual` are registered in hand-written
C++ because they need custom parsing, overload handling, or Python-specific
wrapping. Classes with `binding: excluded` are intentionally not exposed.

## Fields

- `class`: Fully qualified C++ class name. This is the only required field for
  a generated entry. Manual ledger entries may also use a representative
  template-family spelling, such as `ArrayHandleSOA<...>`, when Python exposes a
  family of template instantiations rather than one concrete C++ class.
- `binding`: Optional binding state. Valid values are `manual` and `excluded`.
  Omit this field for generated bindings.
- `base`: Optional generated Python base class. Use
  `viskores::filter::Filter` for generated filters that should inherit common
  filter methods such as `Execute`, active-field selection, output-field naming,
  and field pass-through controls. Only use this when the C++ class really
  derives from that base. A more specific generated base, such as
  `viskores::filter::contour::Contour`, can be used when the derived class
  should inherit those Python methods too.
- `constructors`: Optional explicit constructor signatures. Omit this field for
  a default constructor. Use an empty signature, `()`, when a class needs a
  default constructor plus additional overloads. Constructor arguments use the
  same `name: CxxType` syntax as generated `methods`. Examples include
  `(file_name: std::string)` for reader and writer classes, or
  `(color_table: const viskores::cont::ColorTable&)` when the Python argument is
  another generated/wrapped Viskores class. Constructor arguments may use the
  same converters as method arguments.
- `implicitConstructors`: Optional list of C++ types to expose with
  `nb::init_implicit<T>()`. Use this only when Python should be able to pass
  another wrapped class wherever the generated class is expected, such as
  implicit-function shapes converting to `ImplicitFunctionGeneral`.
- `header`: Optional include path when it cannot be inferred from `class`.
- `guard`: Optional preprocessor condition around the generated include and
  class registration. Use this for classes behind optional Viskores components.
- `pythonName`: Optional public Python class name. By default the generator uses
  the final component of `class`.
- `pythonModule`: Optional Python module name used by generated helpers such as
  `repr`.
- `registrationGroup`: Optional generated-class registration phase. Use `early`
  only when manual bindings need this class to exist before the main generated
  class pass.
- `fieldInputs`: Filter-specific field selection helpers to bind when the common
  `Filter` base methods are not enough. Examples include `PrimaryField` and
  `SecondaryField`.
- `properties`: Simple Set/Get method pairs to bind. For a property named
  `Foo`, the generator binds `SetFoo(value)` and `GetFoo()` by direct C++ method
  pointer. The manifest does not record the C++ type: generated code binds the
  actual methods, and the C++ compiler/nanobind determine argument and return
  conversions from the real `SetFoo` and `GetFoo` signatures. Use `methods`
  instead when the setter is overloaded, needs casts, or needs Python-specific
  parsing.
- `directMethods`: Methods to bind directly by C++ method pointer. Use this for
  simple methods with no custom argument conversion.
- `methods`: Explicit method signatures for generated method bindings. Use this
  for overloaded methods, arguments that need casts from normal Python scalar
  types, defaults, or per-argument conversion. The syntax is
  `MethodName(arg: CxxType, other: CxxType = default)`. The generator infers
  normal Python argument types from common Viskores C++ types, such as
  `viskores::Id` to `long long` and `viskores::Float64` to `double`. Add a
  converter in square brackets only when the Python input shape differs from the
  C++ argument type, for example
  `SetIsoValues(values: std::vector<viskores::Float64> [Float64List])`.
  The generator treats `viskores::Id2` as an exact 2-integer tuple and
  `viskores::Id3`/`viskores::Vec3f`/`viskores::Vec3f_32` as exact 3-value tuples. Available argument
  converters include `Bounds`, `Dimensions`, `Float64List`, `IdList`,
  `NumPyId3Array`, `NumPyVec3fArray`, `Range`, `RangeId3`, `StringList`,
  `UnknownArray`, `Vec3fDefaultOne`, and `Vec3fDefaultZero`.
  Available return converters include `CoordinateSystemData`, which widens the
  coordinate-system-specific uncertain-array return to the public
  `UncertainArrayHandle` instantiation.
- `staticMethods`: Static method signatures using the same syntax and converter
  rules as `methods`.
- `enum`: Top-level enum to register on the module. Use `pythonName` for the
  public Python name and `values` for the enum values to expose.
- `enumOptions`: Optional nanobind enum options. Use `flag` for bitmask-style
  enum operations and `arithmetic` when integer interop is needed.
- `enums`: Scoped enums to register on the generated class. The syntax is
  `EnumName: Value, OtherValue` for nested enums, or
  `full::EnumType as PythonName: Value, OtherValue` when the enum type or
  Python name needs to be explicit.
- `repr`: Optional generated `__repr__` helper.
- `reason`: Required for `manual` and `excluded` entries. Explain the concrete
  reason the class is not generated or exposed. Manual entries should state the
  generator capability that would be needed to automate the binding.

## Adding a Class

1. Choose the manifest file that matches the Viskores component, or add a new
   file named after the component.
2. Add one entry with the fully qualified `class` name.
3. For a normal filter, add `base: viskores::filter::Filter` and list only the
   methods specific to that derived class. Do not list inherited filter methods
   such as `Execute`, `SetActiveField`, `SetOutputFieldName`, or
   `SetFieldsToPass`. For non-filter classes, omit `base` unless there is a
   generated Python base class that should be used.
4. Add `constructors`, `properties`, `directMethods`, `methods`, or
   `fieldInputs` only for behavior that is specific to the class. Prefer
   `methods` for overloads and per-argument conversions, and make the generated
   method names explicit in the manifest.
5. If the class needs Python-specific parsing or behavior that cannot be
   expressed with `properties`, `directMethods`, `methods`, or `fieldInputs`,
   use `binding: manual` and add a concrete `reason`.
6. If the class should not be exposed, use `binding: excluded` and explain what
   users should use instead or what support is missing.
7. Run the manifest audit and build the Python bindings.

Example generated filter entry:

```yaml
- class: viskores::filter::contour::ContourMarchingCells
  base: viskores::filter::Filter
  properties:
    - GenerateNormals
    - MergeDuplicatePoints
  methods:
    - SetIsoValue(value: viskores::Float64)
    - SetIsoValue(index: viskores::Id, value: viskores::Float64)
    - SetIsoValues(values: std::vector<viskores::Float64> [Float64List])
    - GetIsoValue(index: viskores::Id = 0)
    - GetNumberOfIsoValues()
```

Example generated reader entry:

```yaml
- class: viskores::io::VTKDataSetReader
  constructors:
    - (file_name: std::string)
  methods:
    - ReadDataSet() -> viskores::cont::DataSet
```

Example generated source entry:

```yaml
- class: viskores::source::Tangle
  pythonModule: viskores.source
  methods:
    - SetCellDimensions(dims: viskores::Id3)
    - GetCellDimensions() -> viskores::Id3
    - Execute() -> viskores::cont::DataSet
```

Example manual entry:

```yaml
- class: viskores::filter::resampling::Probe
  binding: manual
  reason: Requires geometry dataset wrapping.
```

Useful checks:

```sh
python3 python_bindings/tools/binding_manifest.py \
  --manifest python_bindings/binding_manifest \
  --source-root . \
  report

python3 python_bindings/tools/binding_manifest.py \
  --manifest python_bindings/binding_manifest \
  --source-root . \
  generate --output /tmp/generated_filter_bindings.cxx

ctest --test-dir /path/to/viskores-build \
  -R 'PythonBindingManifestAudit|UnitTestPythonWrapper'
```
