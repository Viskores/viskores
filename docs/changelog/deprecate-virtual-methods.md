# Virtual methods in execution environment deprecated

The use of classes with any virtual methods in the execution environment is
deprecated. Although we had code to correctly build virtual methods on some
devices such as CUDA, this feature was not universally supported on all
programming models we wish to support. Plus, the implementation of virtual
methods is not hugely convenient on CUDA because the virtual methods could
not be embedded in a library. To get around virtual methods declared in
different libraries, all builds had to be static, and a special linking
step to pull in possible virtual method implementations was required.

For these reasons, VTK-m is no longer relying on virtual methods. (Other
approaches like multiplexers are used instead.) The code will be officially
removed in version 2.0. It is still supported in a deprecated sense (you
should get a warning). However, if you want to build without virtual
methods, you can set the `VTKm_NO_DEPRECATED_VIRTUAL` CMake flag, and they
will not be compiled.
