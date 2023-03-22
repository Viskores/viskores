# New Composite Vector filter

The composite vector filter combines multiple scalar fields into a single vector field. Scalar fields are selected as the active input fields, and the combined vector field is set at the output. The current composite vector filter only supports 2d and 3d scalar field composition. Users may use `vtkm::cont::make_ArrayHandleCompositeVector` to execute a more flexible scalar field composition.
