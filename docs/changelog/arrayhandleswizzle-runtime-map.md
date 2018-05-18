# `ArrayHandleSwizzle` component maps are now set at runtime.

Rather than embedding the component map in the template parameters, the swizzle
operation is now defined at runtime using a `vtkm::Vec<vtkm::IdComponent, N>`
that maps the input components to the output components.

This is easier to use and keeps compile times / sizes / memory requirements
down.
