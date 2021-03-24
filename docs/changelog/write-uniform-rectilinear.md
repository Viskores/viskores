# Write uniform and rectilinear grids to legacy VTK files

As a programming convenience, all `vtkm::cont::DataSet` written by
`vtkm::io::VTKDataSetWriter` were written as a structured grid. Although
technically correct, it changed the structure of the data. This meant that
if you wanted to capture data to run elsewhere, it would run as a different
data type. This was particularly frustrating if the data of that structure
was causing problems and you wanted to debug it.

Now, `VTKDataSetWriter` checks the type of the `CoordinateSystem` to
determine whether the data should be written out as `STRUCTURED_POINTS`
(i.e. a uniform grid), `RECTILINEAR_GRID`, or `STRUCTURED_GRID`
(curvilinear).
