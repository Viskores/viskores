# Masks and Scatters Supported for 3D Scheduling

Previous to this change worklets that wanted to use non-default
`vtkm::worklet::Mask` or `vtkm::worklet::Scatter` wouldn't work when scheduled
to run across `vtkm::cont::CellSetStructured` or other `InputDomains` that
supported 3D scheduling.

This restriction was an inadvertent limitation of the VTK-m worklet scheduling
algorithm. Lifting the restriction and providing sufficient information has
been achieved in a manner that shouldn't degrade performance of any existing
worklets.
