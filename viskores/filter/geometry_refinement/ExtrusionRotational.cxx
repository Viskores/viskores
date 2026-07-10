//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/geometry_refinement/ExtrusionAbstract.hxx>
#include <viskores/filter/geometry_refinement/ExtrusionRotational.h>
#include <viskores/filter/geometry_refinement/worklet/ExtrusionRotational.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleTransform.h>

namespace
{

VISKORES_CONT bool NearlyFullSweep(viskores::FloatDefault angle)
{
  constexpr viskores::Float64 tolerance = 1.0e-6;
  const viskores::Float64 fullSweep = viskores::TwoPi<viskores::Float64>();
  const viskores::Float64 absAngle = viskores::Abs(static_cast<viskores::Float64>(angle));
  return viskores::Abs(absAngle - fullSweep) <= tolerance;
}

struct IsPointOnAxis
{
  viskores::Vec3f_64 Axis;
  viskores::Vec3f_64 Center;
  viskores::Float64 Tolerance2;

  template <typename PointType>
  VISKORES_EXEC_CONT bool operator()(const PointType& pointValue) const
  {
    const viskores::Vec3f_64 relativeP = viskores::Vec3f_64{ pointValue } - this->Center;

    const viskores::Float64 dot = viskores::Dot(this->Axis, relativeP);
    const viskores::Vec3f_64 perpendicular = relativeP - (dot * this->Axis);
    const viskores::Float64 perpendicularMagnitude2 = viskores::MagnitudeSquared(perpendicular);

    return perpendicularMagnitude2 <= this->Tolerance2;
  }
};

template <typename CoordinateArrayType>
VISKORES_CONT void ValidateAxisPoints(const CoordinateArrayType& coordinates,
                                      viskores::Vec3f_64 axis,
                                      viskores::Vec3f_64 center,
                                      viskores::Float64 tolerance)
{
  const bool hasAxisPoint = viskores::cont::Algorithm::Reduce(
    viskores::cont::make_ArrayHandleTransform(coordinates,
                                              IsPointOnAxis{ axis, center, tolerance * tolerance }),
    false,
    viskores::LogicalOr{});
  if (hasAxisPoint)
  {
    throw viskores::cont::ErrorFilterExecution(
      "ExtrusionRotational: input contains a point on the rotation axis.");
  }
}

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

VISKORES_CONT ExtrusionRotational::ExtrusionRotational()
{
  this->CloseSweep = NearlyFullSweep(this->SweepAngle);
}

VISKORES_CONT void ExtrusionRotational::SetSweepAngle(viskores::FloatDefault radians)
{
  this->SweepAngle = radians;
  if (!this->CloseSweepExplicitlySet)
  {
    this->CloseSweep = NearlyFullSweep(radians);
  }
}

VISKORES_CONT void ExtrusionRotational::SetCloseSweep(bool close)
{
  this->CloseSweep = close;
  this->CloseSweepExplicitlySet = true;
}

VISKORES_CONT viskores::cont::DataSet ExtrusionRotational::DoExecute(
  const viskores::cont::DataSet& input)
{
  const viskores::Float64 axisMagnitude2 = viskores::MagnitudeSquared(this->Axis);
  if (axisMagnitude2 <= 0)
  {
    throw viskores::cont::ErrorFilterExecution("ExtrusionRotational: axis must be nonzero.");
  }
  if (this->AxisPointTolerance < 0)
  {
    throw viskores::cont::ErrorFilterExecution(
      "ExtrusionRotational: axis point tolerance must be nonnegative.");
  }
  if (this->GetCloseSweep() && !NearlyFullSweep(this->SweepAngle))
  {
    throw viskores::cont::ErrorFilterExecution(
      "ExtrusionRotational: closed sweeps require a full 2*pi sweep angle.");
  }

  const viskores::Float64 inverseAxisMagnitude = viskores::RSqrt(axisMagnitude2);
  const viskores::Vec3f_64 normalizedAxis = this->Axis * inverseAxisMagnitude;

  auto makeCoordinateWorklet = [&](viskores::Id numberOfPoints)
  {
    return viskores::worklet::ExtrusionRotationalCoordinates{
      normalizedAxis, this->Center,         this->SweepAngle, this->GetNumberOfPlanes(),
      numberOfPoints, this->GetCloseSweep()
    };
  };

  auto makeOrientationWorklet = [&]()
  {
    const viskores::Float64 denominator = static_cast<viskores::Float64>(
      this->GetCloseSweep() ? this->GetNumberOfPlanes() : (this->GetNumberOfPlanes() - 1));
    const viskores::Float64 stepAngle =
      static_cast<viskores::Float64>(this->SweepAngle) / denominator;

    return viskores::worklet::OrientExtrudedTriangleConnectivity<
      viskores::worklet::ExtrusionRotationalSweep>{ viskores::worklet::ExtrusionRotationalSweep{
      normalizedAxis, this->Center, viskores::Cos(stepAngle), viskores::Sin(stepAngle) } };
  };

  auto validateCoordinates = [&](const auto& coordinates)
  {
    if (!this->AllowAxisPoints)
    {
      ValidateAxisPoints(coordinates, normalizedAxis, this->Center, this->AxisPointTolerance);
    }
  };

  return this->ExecuteTriangleExtrusion(input,
                                        "ExtrusionRotational",
                                        this->GetCloseSweep(),
                                        makeCoordinateWorklet,
                                        makeOrientationWorklet,
                                        validateCoordinates);
}
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores
