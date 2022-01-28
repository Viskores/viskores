# Add `CreateResult` to `NewFilter` and absorb field mapping

The original version of `Filter` classes had a helper header file named
`CreateResult.h` that had several forms of a `CreateResult` function that
helped correctly create the `DataSet` to be returned from a filter's
`DoExecute`. With the move to the `NewFilter` structure, these functions
did not line up very well with how `DataSet`s should actually be created.

A replacement for these functions have been added as protected helper
methods to `NewFilter` and `NewFilterField`. In addition to moving them
into the filter themselves, the behavior of `CreateResult` has been merged
with the map field to output functionality. The original implementation of
`Filter` did this mapping internally in a different step. The first design
of `NewFilter` required the filter implementer to call a
`MapFieldsOntoOutput` themselves. This new implementation wraps the
functionality of `CreateResult` and `MapFieldsOntoOutput` together so that
the `DataSet` will be created correctly with a single call to
`CreateResult`. This makes it easier to correctly create the output.
