# Consolidate count-to-offset algorithms

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

Part of this change removed unnecessary includes of `Algorithm.h` in
`ArrayHandleGroupVecVariable.h` and `CellSetExplicit.h`. This header had to
be added to some classes that were not including it themselves.
