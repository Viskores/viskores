# Add a CUDA-safe `vtkm::Swap` method.

Added a swap implementation that is safe to call from all backends.

It is not legal to call std functions from CUDA code, and the new
`vtkm::Swap` implements a naive swap when compiled under NVCC while
falling back to a std/ADL swap otherwise.
