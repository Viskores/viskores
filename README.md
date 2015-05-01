## VTK-m ##

One of the biggest recent changes in high-performance computing is the increasing use of accelerators. Accelerators contain processing cores that independently are inferior to a core in a typical CPU, but these cores are replicated and grouped such that their aggregate execution provides a very high computation rate at a much lower power. Current and future CPU processors also require much more explicit parallelism. Each successive version of the hardware packs more cores into each processor, and technologies like hyperthreading and vector operations require even more parallel processing to leverage each core’s full potential.

VTK-m is a toolkit of scientific visualization algorithms for emerging processor architectures. VTK-m supports the fine-grained concurrency for data analysis and visualization algorithms required to drive extreme scale computing by providing abstract models for data and execution that can be applied to a variety of algorithms across many different processor architectures.


## Getting VTK-m ##

The VTK-m repository is located at [http://public.kitware.com/gitweb?p=vtkm.git](http://public.kitware.com/gitweb?p=vtkm.git)

VTK-m dependencies are:


+  [CMake 3.0](http://www.cmake.org/download/)
+  [Boost 1.52.0](http://www.boost.org) or greater
+  [Cuda Toolkit 6+](https://developer.nvidia.com/cuda-toolkit) or [Thrust 1.7+](https://thrust.github.com)

```
git clone git://public.kitware.com/vtkm.git vtkm
mkdir vtkm-build
cd vtkm-build
cmake-gui ../vtkm
```

A detailed walk-through of installing and building VTK-m can be found on our [Contributing page](http://m.vtk.org/index.php/Contributing_to_VTK-m)

