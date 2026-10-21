//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

#include <viskores/Bounds.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores_device
{

namespace detail
{

// The corners of the box of half-size |radius| around one point.
struct PaddedBoxCorners : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn center, FieldIn radius, FieldOut lower, FieldOut upper);

  template <typename CenterType>
  VISKORES_EXEC void operator()(const CenterType& center,
                                viskores::Float32 radius,
                                viskores::Vec3f_64& lower,
                                viskores::Vec3f_64& upper) const
  {
    const viskores::Float64 r = viskores::Abs(static_cast<viskores::Float64>(radius));
    lower = viskores::Vec3f_64(center) - viskores::Vec3f_64(r);
    upper = viskores::Vec3f_64(center) + viskores::Vec3f_64(r);
  }
};

} // namespace detail

// The bounds of the boxes of half-size |radius| around each point, with one
// radius per point. Used by geometry whose extent reaches past its points,
// such as spheres around their centers and cylinders around their end points.
template <typename PointArrayType, typename RadiusArrayType>
viskores::Bounds PaddedBounds(const PointArrayType& points, const RadiusArrayType& radii)
{
  viskores::cont::ArrayHandle<viskores::Vec3f_64> lower;
  viskores::cont::ArrayHandle<viskores::Vec3f_64> upper;
  viskores::cont::Invoker invoke;
  invoke(detail::PaddedBoxCorners{}, points, radii, lower, upper);

  const viskores::cont::ArrayHandle<viskores::Range> lowerRangeArray =
    viskores::cont::ArrayRangeCompute(lower);
  const viskores::cont::ArrayHandle<viskores::Range> upperRangeArray =
    viskores::cont::ArrayRangeCompute(upper);
  const auto lowerRanges = lowerRangeArray.ReadPortal();
  const auto upperRanges = upperRangeArray.ReadPortal();
  return viskores::Bounds(viskores::Range(lowerRanges.Get(0).Min, upperRanges.Get(0).Max),
                          viskores::Range(lowerRanges.Get(1).Min, upperRanges.Get(1).Max),
                          viskores::Range(lowerRanges.Get(2).Min, upperRanges.Get(2).Max));
}

} // namespace viskores_device
