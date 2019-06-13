# VTK-m CUDA kernel scheduling including improved defaults, and user customization

VTK-m now offers a more GPU aware set of defaults for kernel scheduling.
When VTK-m first launches a kernel we do system introspection and determine
what GPU's are on the machine and than match this information to a preset
table of values. The implementation is designed in a way that allows for
VTK-m to offer both specific presets for a given GPU ( V100 ) or for
an entire generation of cards ( Pascal ).

Currently VTK-m offers preset tables for the following GPU's:
- Tesla V100
- Tesla P100

If the hardware doesn't match a specific GPU card we than try to find the
nearest know hardware generation and use those defaults. Currently we offer
defaults for
- Older than Pascal Hardware
- Pascal Hardware
- Volta+ Hardware

Some users have workloads that don't align with the defaults provided by
VTK-m. When that is the cause, it is possible to override the defaults
by binding a custom function to `vtkm::cont::cuda::InitScheduleParameters`.
As shown below:

```cpp
  ScheduleParameters CustomScheduleValues(char const* name,
                                          int major,
                                          int minor,
                                          int multiProcessorCount,
                                          int maxThreadsPerMultiProcessor,
                                          int maxThreadsPerBlock)
  {

    ScheduleParameters params  {
        64 * multiProcessorCount,  //1d blocks
        64,                        //1d threads per block
        64 * multiProcessorCount,  //2d blocks
        { 8, 8, 1 },               //2d threads per block
        64 * multiProcessorCount,  //3d blocks
        { 4, 4, 4 } };             //3d threads per block
    return params;
  }
  vtkm::cont::cuda::InitScheduleParameters(&CustomScheduleValues);
```
