## Enable Unity build ##

VTK-m now partially supports unity builds in a subset its sources files which
are known to take the longer time/memory to build. Particularly, this enables
you to speedup compilation in VTK-m not memory intensive builds (HIP, CUDA) in a
system with sufficient resources.

We use `BATCH` unity builds type and the number of source files per batch can be
controlled by the canonical _CMake_ variable: `CMAKE_UNITY_BUILD_BATCH_SIZE`.

Unity builds requires _CMake_ >= 3.16, if using a older version, unity build
will be disabled a regular build will be performed.
