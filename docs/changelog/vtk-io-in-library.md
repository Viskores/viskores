# Move VTK file readers and writers into vtkm_io

The legacy VTK file reader and writer were created back when VTK-m was a
header-only library. Things have changed and we now compile quite a bit of
code into libraries. At this point, there is no reason why the VTK file
reader/writer should be any different.

Thus, `VTKDataSetReader`, `VTKDataSetWriter`, and several supporting
classes are now compiled into the `vtkm_io` library. Also similarly updated
`BOVDataSetReader` for good measure.

As a side effect, code using VTK-m will need to link to `vtkm_io` if they
are using any readers or writers.
