# Automatically make the fields with the global cell ghosts the cell ghosts

Previously, if you added a cell field to a `DataSet` with a name that was
the same as that returned from `GetGlobalCellFieldName`, it was still only
recognized as a normal field. Now, that field is automatically recognized
as a the cell ghost levels (unless the global cell field name is changed or
a different field is explicitly set as the cell ghost levels).
