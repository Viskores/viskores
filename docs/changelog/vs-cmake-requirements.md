# Visual Studio generator now requires CMake 3.11

We require CMake 3.11 for the Visual Studio generators as the
$<COMPILE_LANGUAGE:> generator expression is not supported on older versions.
