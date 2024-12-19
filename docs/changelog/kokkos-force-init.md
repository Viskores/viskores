## Automatically initialize Kokkos

Calling `vtkm::cont::Initialize()` is supposed to be optional. However, Kokkos
needs to have `Kokkos::initialize()` called before using some devices such as
HIP. To make sure that Kokkos is properly initialized, the VTK-m allocation for
the Kokkos device now checks to see if `Kokkos::is_initialized()` is true. If it
is not, then `vtkm::cont::Initialize()` is called.
