VTK-m 1.5.1 Release Notes
=======================

# Table of Contents
1. [Core](#Core)
     - Variant trivial copy

2. [Build](#Build)
     - GCC 4 trivially copyable
     - MSVC flag fix
     - GCC openmp workaround fix
     - Correct gcc 9 warnings
     - GCC 48 iterator fixes
     - Aligned union check handle gcc 485
     - Update check for aligned union
     - Intel fix
     - Correct msvc 2015 failure
     - GCC 61 ice openmp and optimizations
     - No deprecated nvcc vs

# Core

## Variant trivial copy

The Variant template can hold any type. If it is holding a type that is
non-copyable, then it has to make sure that appropriate constructors,
copiers, movers, and destructors are called.

Previously, these were called even the Variant was holding a trivially
copyable class because no harm no foul. If you were holding a trivially
copyable class and did a `memcpy`, that work work, which should make it
possible to copy between host and device, right?

In theory yes, but in practice no. The problem is that _Cuda_ is
outsmarting the code. It is checking that Variant is not trivially-
copyable by C++ semantics and refusing to push it.

So, change Variant to check to see if all its supported classes are
trivially copyable. If they are, then it use the default constructors,
destructors, movers, and copiers so that C++ recognizes it as trivially
copyable.

    7518d067 Try to fix uninitialized anonymous variable warning
    5b18ffd7 Register Variant as trivially copyable if possible
    16305bd8 Add tests of ArrayHandleMultiplexer on multiple devices

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1898

# Build

## GCC-4 trivially copyable

Although GCC 4.8 and 4.9 claim to be C++11 compliant, there are a few
C++11 features they do not support. One of these features is
`std::is_trivially_copyable`. So on these platforms, do not attempt to use
it. Instead, treat nothing as trivially copyable.

    3b7b21c8 Do not use std::is_trivially_copyable on GCC 4.X

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1900

## Fix MSVC flag

Fix MSVC flags for CUDA builds.

    07b55a95 Fix MSVC flags for CUDA builds.

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1919

## GCC OpenMP workaround fix

There is some behavior of GCC compilers before GCC 9.0 that is
incompatible with the specification of `OpenMP` 4.0. The workaround was
using the workaround any time a GCC compiler >= 9.0 was used. The proper
behavior is to only use the workaround when the GCC compiler is being
used and the version of the compiler is less than 9.0.
Also, switch to using `VTKM_GCC` to check for the GCC compiler instead of
GNUC. The problem with using GNUC is that many other compilers
pretend to be GCC by defining this macro, but in cases like compiler
workarounds it is not accurate.

    033dfe55 Only workaround incorrect GCC behavior for OpenMP on GCC

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1904

## Correct GCC-9 warnings

    870bd1d1 Removed unnecessary increment and decrement from ZFPDecode
    f9860b84 Correct warnings found by gcc-9 in vtkm::Particle

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1935

## GCC-4.8 iterator fixes

The draft C++11 spec that GCC-4.X implemented against had some
defects that made implementing `void_t<...>` tricky.

    83d4d4e4 ArrayPortalToIterators now compiles with GCC-4.X

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1934

## Aligned union check handle GCC

    b36846e4 UnitTestVariant uses VTKM_USING_GLIBCXX_4
    cbf20ac3 Merge branch 'upstream-diy' into aligned_union_check_handle_gcc_485
    ac1a23be diy 2019-12-17 (bb86e1f7)
    201e5c81 Add the gcc 4.8.5 release date to our VTKM_USING_GLIBCXX_4 check

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1930

## Update check for aligned union

Fixes #447 (closed)
This uses a more robust set of checks to determine if `std::aligned_union`
and `std::is_trivially_copyable` exist given the `libstdc++` version value

    2e48d98d Merge branch 'upstream-diy' into update_check_for_aligned_union
    bbd5db31 diy 2019-12-16 (e365b66a)
    269261b9 Handle compiling against 4.X versions of libstdc++

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1928

## Intel fix

    b6b20f08 Use brigand integer sequences on icc.

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1923

## Correct MSVC 2015 failure

    f89672b7 UnitTestFetchArrayTopologyMapIn now compiles with VS2015

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1921

## GCC-61 ice OpenMP and optimizations

    dc86ac20 Avoid a GCC 6.1 compiler regression that occurs when openmp is enabled

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1950

## No deprecated NVCC VS

The NVCC compiler under visual studio seems to give the error attribute does
not apply to any entity when you try to use the [[deprecated]] attribute.
So disable for this compiler configuration.

    fb01d38a Disable deprecated attribute when using nvcc under VS

Merge-request: https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1949
