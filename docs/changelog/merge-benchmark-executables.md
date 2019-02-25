# Merge benchmark executables into a device dependent shared library

VTK-m has been updated to replace old per device benchmark executables with a device
dependent shared library so that it's able to accept a device adapter at runtime through
the "--device=" argument.

