# Put CellLocatorBoundingIntervalHierarchy in vtkm_cont library

All of the methods in CellLocatorBoundingIntervalHierarchy were listed in
header files. This is sometimes problematic with virtual methods. Since
everything implemented in it can just be embedded in a library, move the
code into the vtkm_cont library.

