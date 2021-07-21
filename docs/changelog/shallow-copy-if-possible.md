# Added `ArrayCopyShallowIfPossible`

Often times you have an array of an unknown type (likely from a data set),
and you need it to be of a particular type (or can make a reasonable but
uncertain assumption about it being a particular type). You really just
want a shallow copy (a reference in a concrete `ArrayHandle`) if that is
possible.

`ArrayCopyShallowIfPossible` pulls an array of a specific type from an
`UnknownArrayHandle`. If the type is compatible, it will perform a shallow
copy. If it is not possible, a deep copy is performed to get it to the
correct type.
