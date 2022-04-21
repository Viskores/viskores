# Generalized instantiation

Recently, an instantiation method was added to the VTK-m configuration
files to set up a set of source files that compile instances of a template.
This allows the template instances to be compiled exactly once in separate
build files.

However, the implementation made the assumption that the instantiations
were happening for VTK-m filters. Now that the VTK-m filters are being
redesigned, this assumption is broken.

Thus, the instantiation code has been redesigned to be more general. It can
now be applied to code within the new filter structure. It can also be
applied anywhere else in the VTK-m source code.
