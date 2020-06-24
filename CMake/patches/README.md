# CMake backports

This directory contains backports from newer CMake versions to help support
actually using older CMake versions for building VTK-m. The directory name is the
minimum version of CMake for which the contained files are no longer necessary.
For example, the files under the `3.15` directory are not needed for 3.15 or
3.16, but are for 3.14.
