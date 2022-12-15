# Particle class members are hidden

The member variables of the `vtkm::Particle` classes are now hidden. This
means that external code will not be directly able to access member
variables like `Pos`, `Time`, and `ID`. Instead, these need to be retrieved
and changed through accessor methods.

This follows standard C++ principles. It also helps us future-proof the
classes. It means that we can provide subclasses or alternate forms of
`Particle` that operate differently. It also makes it possible to change
interfaces while maintaining a deprecated interface.
