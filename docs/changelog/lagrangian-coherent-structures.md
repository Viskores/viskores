# Lagrangian Coherent Strucutres (LCS) Filter for VTK-m

The new filter `vtkm::filter::LagrangianStructures` is meant for Finite Time
Lyapunov Exponent (FTLE) calculation using VTK-m.
The filter allows users to calculate FTLE in two ways
1. Provide a dataset with a vector field, which will be used to generate a flow
   map.
2. Provide a dataset containing a flow map, which can be readily used for the
   FTLE field calculation.

The filter returns a dataset with a point field named FTLE.
Is the input is strucutred and an auxiliary grid was not used, the filter will
add the field to the original dataset set, else a new structured dataset is returned.
