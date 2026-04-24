##=============================================================================
##
##  The contents of this file are covered by the Viskores license. See
##  LICENSE.txt for details.
##
##  By contributing to this file, all contributors agree to the Developer
##  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
##
##=============================================================================

# Direct Python API Refactor Plan

The target API should track the Viskores C++ namespace and class structure as closely
as practical while still handling Python-specific interop cleanly.

## API goals

- Prefer direct bindings to real Viskores classes.
- Mirror Viskores namespaces as Python submodules.
- Keep bound Viskores method names in their original C++ spelling.
- Reserve Python-only helper APIs for NumPy conversion and similar interop tasks.
- Avoid demo-specific facade classes as the long-term public API.

## Target package layout

- `viskores.cont`
- `viskores.source`
- `viskores.filter.contour`
- `viskores.filter.field_conversion`
- `viskores.filter.vector_analysis`
- `viskores.filter.scalar_topology`
- `viskores.rendering`

## C++ binding file split

Split the current monolithic implementation by Viskores module boundaries.

- `python_bindings/src/module.cxx`
  - extension initialization and registration glue
- `python_bindings/src/common/*`
  - shared Python helpers, NumPy conversion, error handling
- `python_bindings/src/cont/*`
  - `DataSet`, `PartitionedDataSet`, `ColorTable`, dataset builders
- `python_bindings/src/source/*`
  - `Tangle`, `Wavelet`, and other sources as added
- `python_bindings/src/filter/contour/*`
  - `Contour` and related contour filters
- `python_bindings/src/filter/field_conversion/*`
  - `CellAverage`, `PointAverage`
- `python_bindings/src/filter/vector_analysis/*`
  - `Gradient`, `VectorMagnitude`
- `python_bindings/src/filter/scalar_topology/*`
  - contour-tree-related bindings
- `python_bindings/src/rendering/*`
  - `Camera`, `Actor`, `Scene`, `CanvasRayTracer`, `View3D`, mappers

## Migration order

1. Preserve and improve `DataSet` and `PartitionedDataSet` NumPy interop.
2. Replace Python wrapper `source.Tangle` with a direct binding.
3. Replace Python wrapper `filter.contour.Contour` with a direct binding.
4. Replace Python wrapper `rendering.Camera` and `cont.ColorTable` with direct bindings.
5. Rework `examples/demo/Demo.py` to use the namespace-based API and direct classes.
6. Evaluate which rendering classes can be exposed directly without distorting the API.
