# Support backward compatibility in CMake package

VTK-m development is in a mode where backward compatibility should be
maintained between minor versions of the software. (You may get deprecation
warnings, but things should still work.) To match this behavior, the
generated CMake package now supports finding versions with the same major
release and the same or newer minor release. For example, if an external
program does this

``` cmake
find_package(VTKm 2.1 REQUIRED)
```

then CMake will link to 2.1 (of course) as well as newer minor releases
(e.g., 2.2, 2.3, etc.). It will not, however, match older versions (e.g.,
2.0, 1.9), nor will it match any version after the next major release
(e.g., 3.0).
