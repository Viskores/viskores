# Add atomic free functions

Previously, all atomic functions were stored in classes named
`AtomicInterfaceControl` and `AtomicInterfaceExecution`, which required
you to know at compile time which device was using the methods. That in
turn means that anything using an atomic needed to be templated on the
device it is running on.

That can be a big hassle (and is problematic for some code structure).
Instead, these methods are moved to free functions in the `vtkm`
namespace. These functions operate like those in `Math.h`. Using
compiler directives, an appropriate version of the function is compiled
for the current device the compiler is using.

