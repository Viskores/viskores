## Contour Tree Filter Headers Isolated from Device Code

Some of the contour tree filter headers included internal classes that exposed
device (execution environment) code. Filters should not do that so software
using the filters can avoid compiling with the GPU compiler if they otherwise do
not need it. The contour tree filter code is updated to avoid such extraneous
includes.
