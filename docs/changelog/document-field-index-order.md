# Clarify field index ordering in Doxygen

The fields in a `DataSet` are indexed from `0` to `GetNumberOfFields() - 1`.
It is natural to assume that the fields will be indexed in the order that
they are added, but they are not. Rather, the indexing is arbitrary and can
change any time a field is added to the dataset.

To make this more clear, Doxygen documentation is added to the `DataSet`
methods to inform users to not make any assumptions about the order of
field indexing.
