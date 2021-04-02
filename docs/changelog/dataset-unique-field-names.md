# DataSet now only allows unique field names

When you add a `vtkm::cont::Field` to a `vtkm::cont::DataSet`, it now
requires every `Field` to have a unique name. When you attempt to add a
`Field` to a `DataSet` that already has a `Field` of the same name and
association, the old `Field` is removed and replaced with the new `Field`.

You are allowed, however, to have two `Field`s with the same name but
different associations. For example, you could have a point `Field` named
"normals" and also have a cell `Field` named "normals" in the same
`DataSet`.

This new behavior matches how VTK's data sets manage fields.

The old behavior allowed you to add multiple `Field`s with the same name,
but it would be unclear which one you would get if you asked for a `Field`
by name.
