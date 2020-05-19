# Avoid raising errors when operating on cells

Cell operations like interpolate and finding parametric coordinates can
fail under certain conditions. The previous behavior was to call
`RaiseError` on the worklet. By design, this would cause the worklet
execution to fail. However, that makes the worklet unstable for a conditin
that might be relatively common in data. For example, you wouldn't want a
large streamline worklet to fail just because one cell was not found
correctly.

To work around this, many of the cell operations in the execution
environment have been changed to return an error code rather than raise an
error in the worklet.

## Error Codes

To support cell operations efficiently returning errors, a new enum named
`vtkm::ErrorCode` is available. This is the current implementation of
`ErrorCode`.

``` cpp
enum class ErrorCode
{
  Success,
  InvalidShapeId,
  InvalidNumberOfPoints,
  WrongShapeIdForTagType,
  InvalidPointId,
  InvalidEdgeId,
  InvalidFaceId,
  SolutionDidNotConverge,
  MatrixFactorizationFailed,
  DegenerateCellDetected,
  MalformedCellDetected,
  OperationOnEmptyCell,
  CellNotFound,

  UnknownError
};
```

A convenience function named `ErrorString` is provided to make it easy to
convert the `ErrorCode` to a descriptive string that can be placed in an
error.

## New Calling Specification

Previously, most execution environment functions took as an argument the
worklet calling the function. This made it possible to call `RaiseError` on
the worklet. The result of the operation was typically returned. For
example, here is how the _old_ version of interpolate was called.

``` cpp
FieldType interpolatedValue =
  vtkm::exec::CellInterpolate(fieldValues, pcoord, shape, worklet);
```

The worklet is now no longer passed to the function. It is no longer needed
because an error is never directly raised. Instead, an `ErrorCode` is
returned from the function. Because the `ErrorCode` is returned, the
computed result of the function is returned by passing in a reference to a
variable. This is usually placed as the last argument (where the worklet
used to be). here is the _new_ version of how interpolate is called.

``` cpp
FieldType interpolatedValue;
vtkm::ErrorCode result =
  vtkm::exec::CellInterpolate(fieldValues, pcoord, shape, interpolatedValue);
```

The success of the operation can be determined by checking that the
returned `ErrorCode` is equal to `vtkm::ErrorCode::Success`.
