# GenerateIds filter

This filter adds a pair of fields to a `DataSet` which mirror the
indices of the points and cells, respectively. These fields are useful
for tracking the provenance of the elements of a `DataSet` as it gets
manipulated by the filters. It is also convenient for adding indices to
operations designed for fields and for testing purposes.
