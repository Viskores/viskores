# Deprecate the GetCounts() method in Keys objects

The `vtkm::worklet::Keys` object held a `SortedValuesMap` array, an
`Offsets` array, a `Counts` array, and (optionally) a `UniqueKeys` array.
Of these, the `Counts` array is redundant because the counts are trivially
computed by subtracting adjacent entries in the offsets array. This pattern
shows up a lot in VTK-m, and most places we have moved to removing the
counts and just using the offsets.

This change removes the `Count` array from the `Keys` object. Where the
count is needed internally, adjacent offsets are subtracted. The deprecated
`GetCounts` method is implemented by copying values into a new array.
