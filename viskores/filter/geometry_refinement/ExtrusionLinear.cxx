//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/geometry_refinement/ExtrusionAbstract.hxx>
#include <viskores/filter/geometry_refinement/ExtrusionLinear.h>
#include <viskores/filter/geometry_refinement/worklet/ExtrusionLinear.h>

namespace viskores
{
namespace filter
{
namespace geometry_refinement
{

VISKORES_CONT viskores::cont::DataSet ExtrusionLinear::DoExecute(
  const viskores::cont::DataSet& input)
{
  const viskores::Vec3f_64 direction{ this->Direction };
  const viskores::Float64 directionMagnitude2 = viskores::MagnitudeSquared(direction);
  if (directionMagnitude2 <= 0)
  {
    throw viskores::cont::ErrorFilterExecution("ExtrusionLinear: direction must be nonzero.");
  }

  const viskores::Float64 inverseDirectionMagnitude = viskores::RSqrt(directionMagnitude2);
  const viskores::Vec3f_64 normalizedDirection64 = direction * inverseDirectionMagnitude;
  const viskores::Vec3f normalizedDirection(
    static_cast<viskores::FloatDefault>(normalizedDirection64[0]),
    static_cast<viskores::FloatDefault>(normalizedDirection64[1]),
    static_cast<viskores::FloatDefault>(normalizedDirection64[2]));
  const viskores::Vec3f sweep = normalizedDirection * this->Distance;

  auto makeCoordinateWorklet = [&](viskores::Id numberOfPoints)
  {
    return viskores::worklet::ExtrusionLinearCoordinates{
      normalizedDirection, this->Distance, this->GetNumberOfPlanes(), numberOfPoints
    };
  };

  auto makeOrientationWorklet = [&]()
  {
    return viskores::worklet::OrientExtrudedTriangleConnectivity<
      viskores::worklet::ExtrusionLinearSweep>{ viskores::worklet::ExtrusionLinearSweep{ sweep } };
  };

  auto validateCoordinates = [](const auto&) {};

  return this->ExecuteTriangleExtrusion(input,
                                        "ExtrusionLinear",
                                        false,
                                        makeCoordinateWorklet,
                                        makeOrientationWorklet,
                                        validateCoordinates);
}
} // namespace geometry_refinement
} // namespace filter
} // namespace viskores
