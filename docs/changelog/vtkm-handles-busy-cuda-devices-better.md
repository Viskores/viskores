# VTK-m CUDA detection properly handles busy devices

When an application that uses VTK-m is first launched it will
do a check to see if CUDA is supported at runtime. If for
some reason that CUDA card is not allowing kernel execution
VTK-m would report the hardware doesn't have CUDA support.

This was problematic as was over aggressive in disabling CUDA
support for hardware that could support kernel execution in
the future. With the fact that every VTK-m worklet is executed
through a TryExecute it is no longer necessary to be so
aggressive in disabling CUDA support.

Now the behavior is that VTK-m considers a machine to have
CUDA runtime support if it has 1+ GPU's of Kepler or
higher hardware (SM_30+).

