# Viskores 1.1.0 Release Notes

## Table of Contents
1. [Core](#core)
   - Support TypeTraits for reference types
   - Enable more types for Clamp function
   - Update to C++17
   - Added CountAllCells and FindAllCells to Cell Locators
2. [Arrays](#arrays)
   - Added ArrayHandleSOAStride
   - Added template constructor to MaskSelect
   - Extract Arrays to ArrayHandleSOAStride
3. [Bug Fixes](#bug-fixes)
   - Fixed MapperVolume blending
   - Fixed Clip cellOffset increment
   - Fixed CI Warnings

## Core

### Support TypeTraits for reference types
Viskores now supports passing reference types to `viskores::TypeTraits`.
Previously, if you used `viskores::TypeTraits` with a reference type (such as
`viskores::TypeTraits<viskores::Id&>`) you would get the "default"
implementation, which would state that the type was unknown.

Now when you use `viskores::TypeTraits` with a reference type, you get the
traits for the type without the reference. This feature is important when
implementing templated methods where a templated type can be a reference,
avoiding the need to remove references from templated types.

### Enable more types for Clamp function
The Viskores `Clamp` function now works with any numeric type. Previously it
only worked with basic floating point types (i.e., `viskores::Float32` and
`viskores::Float64`). It is now templated to work with any numeric type and can
operate on `Vec` and `Vec`-like types. Furthermore, it is possible to mix the
types of the arguments without ambiguous overloading.

Documentation for `Clamp` is now included in the user's guide.

### Update to C++17
The minimum required version of Viskores has been bumped up to C++17. While
Viskores should work on newer C++ versions, this is not extensively tested.
Note that Kokkos has moved to requiring C++20, so compiling the Kokkos driver
will require that minimum compiler.

### Added CountAllCells and FindAllCells to Cell Locators
Two new methods have been added to all cell locators:
- `CountAllCells` returns the number of cells that contain a given point
- `FindAllCells` returns the IDs and parametric coordinates of all cells that
  contain a given point

These functions are intended for use with datasets where cells may overlap and
are not typically useful for standard non-overlapping meshes.

## Arrays

### Added ArrayHandleSOAStride
Added a new array type named `ArrayHandleSOAStride` that holds each of its
components in a separate array. Unlike `ArrayHandleSOA`, it represents each
component as an `ArrayHandleStride` rather than a basic array, allowing it to
represent most arrays with values of a fixed vector length.

This new array type can be used similarly to `ArrayHandleRecombineVec` but
requires the Vec length to be determined at compile time. While this means it
cannot represent data with varying component numbers, it allows for simple
`Vec` representation when the number of components is fixed.

### Added template constructor to MaskSelect
The `viskores::worklet::MaskSelect` constructor has been enhanced with a
`MaskSelectTemplate` class that provides a templated version of the
constructor. This allows for efficient construction of masks from selection
arrays with other types such as `viskores::cont::ArrayHandleTransform`.

### Extract Arrays to ArrayHandleSOAStride
`UnknownArrayHandle` now includes a new method `ExtractArrayWithValueType` that
extracts an array with a given `ValueType` regardless of the storage. The
method extracts data to an `ArrayHandleSOAStride` by extracting each component
and collecting them in that array type.

Additionally, most arrays can now be pulled from an `UnknownArrayHandle` using
`AsArrayHandle` with an `ArrayHandleSOAStride` as long as the value types
match. `StorageTagSOAStride` has been added to the list of default storage
types to check.

## Bug Fixes

### Fixed MapperVolume blending
Fixed an issue where `MapperVolume` was writing incorrect depth values to the
buffer when rendering multiple partitions. This was resolved by adding an
option to the `Canvas::WriteToCanvas` method that prevents the depth buffer
from being updated.

### Fixed Clip cellOffset increment
Resolved a bug in the redesigned Clip filter where cellOffset was not being
incremented correctly, causing output cells to be incorrectly placed.
Additional improvements include:
- Proper resource release
- Optimized ScanExclusive usage to operate only on needed batches of points/cells

### Fixed CI Warnings
Addressed multiple compiler warnings identified by the new CI builds of
Viskores that were not present in previous builds.
