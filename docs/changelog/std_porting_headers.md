# Porting layer for future std features

Currently, VTK-m is using C++11. However, it is often useful to use
features in the `std` namespace that are defined for C++14 or later. We can
provide our own versions (sometimes), but it is preferable to use the
version provided by the compiler if available.

There were already some examples of defining portable versions of C++14 and
C++17 classes in a `vtkmstd` namespace, but these were sprinkled around the
source code.

There is now a top level `vtkmstd` directory and in it are header files
that provide portable versions of these future C++ classes. In each case,
preprocessor macros are used to select which version of the class to use.
