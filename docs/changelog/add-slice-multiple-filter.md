# Adding SliceMultiple filter

The SliceMultiple filter can accept multiple implicit functions and output a merged dataset. This filter is added to the filter/contour. The code of this filter is adapted from vtk-h. The mechanism that merges multiple datasets in this filter is supposed to support more general datasets and work a separate filter in the future.
