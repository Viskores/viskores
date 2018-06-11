# VTK-m Worklets now execute on Cuda using grid stride loops

Previously VTK-m Worklets used what is referred to as a monolithic kernel
pattern for worklet execution. This assumes a single large grid of threads
to process an entire array in a single pass. This resulted in launches that
looked like:

```cpp
template<typename F>
void TaskSingular(F f, vtkm::Id end)
{
  const vtkm::Id index = static_cast<vtkm::Id>(blockDim.x * blockIdx.x + threadIdx.x);
  if (index < end)
  {
    f(index);
  }  
}

Schedule1DIndexKernel<TaskSingular><<<totalBlocks, 128, 0, cudaStreamPerThread>>>(
       functor, numInstances);
```

This was problematic as it had the drawbacks of:
- Not being able to reuse any infrastructure between kernel executions.
- Harder to tune performance based on the current hardware.

The solution was to move to a grid stride loop strategy with a block size 
based off the number of SM's on the executing GPU. The result is something
that looks like:

```cpp
template<typename F>
void TaskStrided(F f, vtkm::Id end)
{
  const vtkm::Id start = blockIdx.x * blockDim.x + threadIdx.x;
  const vtkm::Id inc = blockDim.x * gridDim.x;
  for (vtkm::Id index = start; index < end; index += inc)
  {
    f(index);
  }  
}
Schedule1DIndexKernel<TaskStrided><<<32*numSMs, 128, 0, cudaStreamPerThread>>>(
       functor, numInstances);
```

 With a loop stride equal to grid size we maintain the optimal memory
 coalescing patterns as we had with the monolithic version. These changes
 also allow VTK-m to optimize TaskStrided so that it can reuse infrastructure
 between iterations.
