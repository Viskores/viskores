# CMake 3.8 Required to build VTK-m

While VTK-m has always required a fairly recent version
of CMake when building for Visual Studio, or if OpenMP or 
CUDA are enabled, it has supported building with the TBB
device with CMake 3.3.

Given the fact that our primary consumer (VTK) has moved
to require CMake 3.8, it doesn't make sense to require
CMake 3.3 and we have moved to a minimum of 3.8.
