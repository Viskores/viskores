//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_exec_internal_CellLocatorHelpers_h
#define viskores_exec_internal_CellLocatorHelpers_h

#include <viskores/Bounds.h>
#include <viskores/VecTraits.h>

#include <viskores/exec/CellInside.h>
#include <viskores/exec/ParametricCoordinates.h>

namespace viskores
{
namespace exec
{
namespace internal
{

template <typename PointsVecType>
VISKORES_EXEC viskores::Bounds ComputeCellBounds(const PointsVecType& points)
{
  const auto numPoints = viskores::VecTraits<PointsVecType>::GetNumberOfComponents(points);

  viskores::Bounds bounds;
  for (viskores::IdComponent index = 0; index < numPoints; ++index)
    bounds.Include(points[index]);

  return bounds;
}

// TODO: This function may return false positives for non-3D cells because
// the bounds test projects the point onto the cell. The parametric-coordinate
// test still determines whether the point is inside the cell itself.
template <typename CellShapeTag, typename PointsVecType>
VISKORES_EXEC viskores::ErrorCode CellLocatorPointInsideCell(const viskores::Vec3f& point,
                                                             CellShapeTag cellShape,
                                                             const PointsVecType& cellPoints,
                                                             viskores::Vec3f& pCoords,
                                                             bool& inside)
{
  using NativeCoordinatesType = typename viskores::VecTraits<PointsVecType>::ComponentType;

  const NativeCoordinatesType nativePoint(point);
  NativeCoordinatesType nativePCoords;
  const auto bounds = viskores::exec::internal::ComputeCellBounds(cellPoints);
  if (!bounds.Contains(nativePoint))
  {
    inside = false;
    return viskores::ErrorCode::Success;
  }

  VISKORES_RETURN_ON_ERROR(viskores::exec::WorldCoordinatesToParametricCoordinates(
    cellPoints, nativePoint, cellShape, nativePCoords));
  inside = viskores::exec::CellInside(nativePCoords, cellShape);
  pCoords = viskores::Vec3f(nativePCoords);
  return viskores::ErrorCode::Success;
}

} // namespace internal
} // namespace exec
} // namespace viskores

#endif // viskores_exec_internal_CellLocatorHelpers_h
