## NewFilterField

As part of the New Filter Interface Design, `FilterField`, `FilterDataSet`
and `FilterDataSetWithField` are now refactored into a single
`NewFilterField`. A `NewFilterField` takes an input `DataSet` with some
`Fields`, operates on the input `CellSet` and/or `Field` and generates an
output DataSet, possibly with a new `CellSet` and/or `Field`.

Unlike the old `FilterField`, `NewFilterField` can support arbitrary number
of *active* `Field`s. They are set by the extended `SetActiveField` method
which now also takes an integer index.

`SupportedType` and `Policy` are no longer supported or needed by
`NewFilterField`. Implementations are given full responsibility of extracting
an `ArrayHandle` with proper value and storage type list from an input `Field`
(through a variant of `CastAndCall`). Automatic type conversion from unsupported
value types to `FloatDefault` is also added to UnknownArrayHandle. See
`DotProduct::DoExecute` for an example on how to use the new facility.
