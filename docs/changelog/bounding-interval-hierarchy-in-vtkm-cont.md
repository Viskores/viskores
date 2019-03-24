# Put CellLocatorBoundingIntervalHierarchy in vtkm_cont library

All of the methods in CellLocatorBoundingIntervalHierarchy were listed in
header files. This is sometimes problematic with virtual methods. Since
everything implemented in it can just be embedded in a library, move the
code into the vtkm_cont library.

These changes caused some warnings in clang to show up based on virtual
methods in other cell locators. Hence, the rest of the cell locators
have also had some of their code moved to vtkm_cont.
