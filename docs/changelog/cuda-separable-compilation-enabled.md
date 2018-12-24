# VTK-m now requires CUDA separable compilation to build

With the introduction of `vtkm::cont::ArrayHandleVirtual` and the related infrastructure, vtk-m now
requires that all CUDA code be compiled using separable compilation ( -rdc ).
