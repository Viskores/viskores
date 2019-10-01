# `CellSetExplicit` Refactoring

The `CellSetExplicit` class has been refactored to remove the `NumIndices`
array. This information is now derived from the `Offsets` array, which has
been changed to contain `[numCells + 1]` entries.

```
Old Layout:
-----------
NumIndices:   [  2,  4,  3,  3,  2 ]
IndexOffset:  [  0,  2,  6,  9, 12 ]
Connectivity: [  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 ]

New Layout:
-----------
Offsets:      [  0,  2,  6,  9, 12, 14 ]
Connectivity: [  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 ]
```

This will reduce the memory overhead of the cellset by roughly `[numCells * 4]`
bytes.

The `IndexOffset` array / typedefs / methods have been renamed to `Offsets` for
brevity and consistency (similar members were plural, e.g. `Shapes`).

The `NumIndices` array can be recovered from the `Offsets` array by using an
`ArrayHandleDecorator`. This is done automatically by the
`CellSetExplicit::GetNumIndicesArray` method.

The `CellSetExplicit::Fill` signature has changed to remove `numIndices` as a
parameter and to require the `offsets` array as a non-optional argument. To
assist in porting old code, an `offsets` array can be generated from
`numIndices` using the new `vtkm::cont::ConvertNumIndicesToOffsets` methods,
defined in `CellSetExplicit.h`.

```
vtkm::Id numPoints = ...;
auto cellShapes = ...;
auto numIndices = ...;
auto connectivity = ...;
vtkm::cont::CellSetExplicit<> cellSet = ...;

// Old:
cellSet.Fill(numPoints, cellShapes, numIndices, connectivity);

// New:
auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numIndices);
cellSet.Fill(numPoints, cellShapes, connectivity, offsets);
```

Since the `offsets` array now contains an additional offset at the end, it
cannot be used directly with `ArrayHandleGroupVecVariable` with the cellset's
`connectivity` array to create an array handle containing cell definitions.
This now requires an `ArrayHandleView` to trim the last value off of the array:

```
vtkm::cont::CellSetExplicit<> cellSet = ...;
auto offsets = cellSet.GetOffsetsArray(vtkm::TopologyElementTagCell{},
                                       vtkm::TopologyElementTagPoint{});
auto conn = cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{},
                                         vtkm::TopologyElementTagPoint{});

// Old:
auto cells = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

// New:
const vtkm::Id numCells = offsets.GetNumberOfValues - 1;
auto offsetsTrim = vtkm::cont::make_ArrayHandleView(offsets, 0, numCells);
auto cells = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);
```
