# Move VTK file readers and writers into vtkm_io

The legacy VTK file reader and writer were created back when VTK-m was a
header-only library. Things have changed and we now compile quite a bit of
code into libraries. At this point, there is no reason why the VTK file
reader/writer should be any different.

Thus, `VTKDataSetReader`, `VTKDataSetWriter`, and several supporting
classes are now compiled into the `vtk_io` library.

As a side effect, code using VTK-m will need to link to `vtk_io` if they
are using any readers or writers.
