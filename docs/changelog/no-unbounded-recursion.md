# Remove unbounded recursion

GPU device compilers like to determine the stack size needed for a called
kernel. This is only possible if there is no recursive function calls on
the device or at least recursive calls where the termination cannot be
found at compile time.

Device compilers do not particularly like that. We have been getting around
this with CUDA by turning of warnings about stack sizes and setting a large
stack size during a call (which works but is dangerous). More restrictive
devices might not allow recursive calls at all.

To fix this, we will avoid recursive calls in execution environment
(device) code. All such warnings are turned on.

Because of this, we also should not have to worry about lengthening the
stack size, so that code is also removed.
