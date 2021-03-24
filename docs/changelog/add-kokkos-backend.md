# Add Kokkos backend

Adds a new device backend `Kokkos` which uses the kokkos library for parallelism.
User must provide the kokkos build and Vtk-m will use the default configured execution
space.
