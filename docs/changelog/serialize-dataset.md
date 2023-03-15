# Simplified serialization of DataSet objects

`vtkm::cont::DataSet` is a dynamic object that can hold cell sets and
fields of many different types, none of which are known until runtime. This
causes a problem with serialization, which has to know what type to compile
the serialization for, particularly when unserializing the type at the
receiving end. The original implementation "solved" the problem by creating
a secondary wrapper object that was templated on types of field arrays and
cell sets that might be serialized. This is not a great solution as it
punts the problem to algorithm developers.

This problem has been completely solved for fields, as it is possible to
serialize most types of arrays without knowing their type now. You still
need to iterate over every possible `CellSet` type, but there are not that
many `CellSet`s that are practically encountered. Thus, there is now a
direct implementation of `Serialization` for `DataSet` that covers all the
data types you are likely to encounter.

The old `SerializableDataSet` has been deprecated. In the unlikely event an
algorithm needs to transfer a non-standard type of `CellSet` (such as a
permuted cell set), it can use the replacement `DataSetWithCellSetTypes`,
which just specifies the cell set types.
