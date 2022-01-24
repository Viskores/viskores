# Make ArrayCopy not depend on a device compiler

Rather than require `ArrayCopy` to create special versions of copy for
all arrays, use a precompiled versions. This should speed up compiles,
reduce the amount of code being generated, and require the device
compiler on fewer source files.

There are some cases where you still need to copy arrays that are not
well supported by the precompiled versions in `ArrayCopy`. (It will
always work, but the fallback is very slow.) In this case, you will want
to switch over to `ArrayCopyDevice`, which has the old behavior.
