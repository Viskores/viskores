# DataSet only has a single vtkm::cont::CellSet

Multiple `vtkm::cont::CellSets` on a datasets increased the
complexity of using VTK-m correctly without any significant
benefits.

It had the effect that `vtkm::cont::Fields` that representing
cell fields needed to be associated with a given cellset. This
has to be a loose coupling to allow for filters to generate
new output cellsets. At the same time it introduced errors when
that output had a different name.

It raised questions about how should filters propagate cell fields.
Should a filter drop all cell fields not associated with the active
CellSet, or is that too aggressive given the fact that maybe the
algorithm just mistakenly named the field, or the IO routine added
a field with the wrong cellset name.

It increased the complexity of filters, as the developer needed to
determine if the algorithm should support execution on a single `CellSet` or
execution over all `CellSets`. 

Given these issues it was deemed that removing multiple `CellSets` was
the correct way forward. People using multiple `CellSets` will need to
move over to `vtkm::cont::MultiBlock` which supports shared points and
fields between multiple blocks.



