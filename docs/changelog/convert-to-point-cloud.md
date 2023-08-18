# Added a `ConvertToPointCloud` filter

This filter takes a `DataSet` and returns a point cloud representation that
has a vertex cell associated with each point in it. This is useful for
filling in a `CellSet` for data that has points but no cells. It is also
useful for operations in which you want to throw away the cell geometry and
operate on the data as a collection of disparate points.
