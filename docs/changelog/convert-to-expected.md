 # Add ability to convert fields to known types

In VTK-m we have a constant tension between minimizing the number of
types we have to compile for (to reduce compile times and library size)
and maximizing the number of types that our filters support.
Unfortunately, if you don't compile a filter for a specific array type
(value type and storage), trying to run that filter will simply fail.

To compromise between the two, added methods to `DataSet` and `Field`
that will automatically convert the data in the `Field` arrays to a type
that VTK-m will understand. Although this will cause an extra data copy,
it will at least prevent the program from failing, and thus make it more
feasible to reduce types.
