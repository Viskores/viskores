Add ability to test exact neighbor offset locations in BoundaryState.

The following methods:
```
BoundaryState::InXBoundary
BoundaryState::InYBoundary
BoundaryState::InZBoundary
BoundaryState::InBoundary
```

have been renamed to:

```
BoundaryState::IsRadiusInXBoundary
BoundaryState::IsRadiusInYBoundary
BoundaryState::IsRadiusInZBoundary
BoundaryState::IsRadiusInBoundary
```

to distinguish them from the new methods:

```
BoundaryState::IsNeighborInXBoundary
BoundaryState::IsNeighborInYBoundary
BoundaryState::IsNeighborInZBoundary
BoundaryState::IsNeighborInBoundary
```

which check a specific neighbor sample offset instead of a full radius.

The method `BoundaryState::ClampNeighborIndex` has also been added, which clamps
a 3D neighbor offset vector to the dataset boundaries.

This allows iteration through only the valid points in a neighborhood using
either of the following patterns:

Using `ClampNeighborIndex` to restrict the iteration space:
```
struct MyWorklet : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename InNeighborhoodT, typename OutDataT>
  VTKM_EXEC void operator()(const InNeighborhoodT& inData,
                            const vtkm::exec::BoundaryState &boundary,
                            OutDataT& outData) const
  {
    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex({-10, -10, -10});
    const auto maxRadius = boundary.ClampNeighborIndex({10, 10, 10});

    for (vtkm::IdComponent k = minRadius[2]; k <= maxRadius[2]; ++k)
    {
      for (vtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
      {
        for (vtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
        {
          outData = doSomeConvolution(i, j, k, outdata, inData.Get(i, j, k));
        }
      }
    }
  }
};
```

or, using `IsNeighborInBoundary` methods to skip out-of-bounds loops:

```
struct MyWorklet : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename InNeighborhoodT, typename OutDataT>
  VTKM_EXEC void operator()(const InNeighborhoodT& inData,
                            const vtkm::exec::BoundaryState &boundary,
                            OutDataT& outData) const
  {
    for (vtkm::IdComponent k = -10; k <= 10; ++k)
    {
      if (!boundary.IsNeighborInZBoundary(k))
      {
        continue;
      }

      for (vtkm::IdComponent j = -10; j <= 10; ++j)
      {
        if (!boundary.IsNeighborInYBoundary(j))
        {
          continue;
        }

        for (vtkm::IdComponent i = -10; i <= 10; ++i)
        {
          if (!boundary.IsNeighborInXBoundary(i))
          {
            continue;
          }

          outData = doSomeConvolution(i, j, k, outdata, inData.Get(i, j, k));
        }
      }
    }
  }
};
```

The latter is useful for implementing a convolution that substitutes a constant
value for out-of-bounds indices:

```
struct MyWorklet : public vtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename InNeighborhoodT, typename OutDataT>
  VTKM_EXEC void operator()(const InNeighborhoodT& inData,
                            const vtkm::exec::BoundaryState &boundary,
                            OutDataT& outData) const
  {
    for (vtkm::IdComponent k = -10; k <= 10; ++k)
    {
      for (vtkm::IdComponent j = -10; j <= 10; ++j)
      {
        for (vtkm::IdComponent i = -10; i <= 10; ++i)
        {
          if (boundary.IsNeighborInBoundary({i, j, k}))
          {
            outData = doSomeConvolution(i, j, k, outdata, inData.Get(i, j, k));
          }
          else
          { // substitute zero for out-of-bounds samples:
            outData = doSomeConvolution(i, j, k, outdata, 0);
          }
        }
      }
    }
  }
};
```
