## Add Python bindings for DataSet, Field, CoordinateSystem, and DataSetBuilderUniform

The Viskores Python bindings now expose the core DataSet types needed to
build and inspect datasets from Python:

* `viskores.cont.FieldAssociation` — enum with values `Any`, `WholeDataSet`,
  `Points`, `Cells`, `Partitions`, and `Global`.

* `viskores.cont.Field(name, association, data)` — wraps a
  `UnknownArrayHandle` with a name and association. `.GetData()` returns the
  `UnknownArrayHandle`; `.asnumpy()` is a convenience shortcut.

* `viskores.cont.CoordinateSystem(name, data)` — constructs a coordinate
  system from a name and a `UnknownArrayHandle`.

* `viskores.cont.DataSet` — exposes `AddField`, `AddPointField`,
  `AddCellField`, `HasField`, `GetField`, `AddCoordinateSystem`,
  `GetCoordinateSystem`, and the `GetNumberOf{Fields,Points,Cells,
  CoordinateSystems}` accessors.

* `viskores.cont.DataSetBuilderUniform.create(dimensions)` — creates a
  uniform rectilinear dataset. Pass a list of 1, 2, or 3 integer dimensions
  (number of points per axis); origin is `[0,0,0]` and spacing is `[1,1,1]`.
