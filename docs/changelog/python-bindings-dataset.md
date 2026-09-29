## Add Python bindings for DataSet, Field, CoordinateSystem, and DataSetBuilderUniform

The Viskores Python bindings now expose the core DataSet types needed to
build and inspect datasets from Python:

* `viskores.Range` and `viskores.Bounds` — mirror `viskores::Range` and
  `viskores::Bounds` with read/write `Min`/`Max` and `X`/`Y`/`Z` members plus
  `IsNonEmpty()`, `Length()`, and `Center()` accessors.

* `viskores.cont.FieldAssociation` — enum with values `Any`, `WholeDataSet`,
  `Points`, `Cells`, `Partitions`, and `Global`.

* `viskores.cont.Field(name, association, data)` — wraps a
  `UnknownArrayHandle` with a name and association. `.GetData()` returns the
  `UnknownArrayHandle`; `.asnumpy()` is a convenience shortcut.

* `viskores.cont.CoordinateSystem(name, data)` — constructs a coordinate
  system from a name and a `UnknownArrayHandle`. `.GetBounds()` returns the
  axis-aligned bounds as a `viskores.Bounds`.

* `viskores.cont.DataSet` — exposes `AddField`, `AddPointField`,
  `AddCellField`, `HasField`, `GetField`, `AddCoordinateSystem`,
  `GetCoordinateSystem`, and the `GetNumberOf{Fields,Points,Cells,
  CoordinateSystems}` accessors. Indexed accessors validate the index and
  raise an exception when it is out of range.

* `viskores.cont.DataSetBuilderUniform.Create(dimensions)` — creates a
  uniform rectilinear dataset. Pass a list of 1, 2, or 3 integer dimensions
  (number of points per axis, each >= 1); origin is `[0,0,0]` and spacing is
  `[1,1,1]`. As in the C++ API, a dimension of 1 collapses that axis,
  lowering the dataset's dimensionality.
