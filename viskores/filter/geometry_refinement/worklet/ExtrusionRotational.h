//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_worklet_ExtrusionRotational_h
#define viskores_worklet_ExtrusionRotational_h

#include <viskores/Math.h>
#include <viskores/filter/geometry_refinement/worklet/Extrusion.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{

class ExtrusionRotationalSweep
{
public:
  VISKORES_CONT
  ExtrusionRotationalSweep(viskores::Vec3f_64 axis,
                           viskores::Vec3f_64 center,
                           viskores::Float64 cosStep,
                           viskores::Float64 sinStep)
    : Axis(axis)
    , Center(center)
    , CosStep(cosStep)
    , SinStep(sinStep)
  {
  }

  template <typename ComponentType>
  VISKORES_EXEC viskores::Vec<ComponentType, 3> operator()(
    const viskores::Vec<ComponentType, 3>& point0,
    const viskores::Vec<ComponentType, 3>& point1,
    const viskores::Vec<ComponentType, 3>& point2) const
  {
    const viskores::Vec<ComponentType, 3> axis(static_cast<ComponentType>(this->Axis[0]),
                                               static_cast<ComponentType>(this->Axis[1]),
                                               static_cast<ComponentType>(this->Axis[2]));
    const viskores::Vec<ComponentType, 3> center(static_cast<ComponentType>(this->Center[0]),
                                                 static_cast<ComponentType>(this->Center[1]),
                                                 static_cast<ComponentType>(this->Center[2]));

    const viskores::Vec<ComponentType, 3> centroid =
      (point0 + point1 + point2) / static_cast<ComponentType>(3);
    const viskores::Vec<ComponentType, 3> shifted = centroid - center;
    const viskores::Vec<ComponentType, 3> parallel = viskores::Dot(axis, shifted) * axis;
    const viskores::Vec<ComponentType, 3> perpendicular = shifted - parallel;
    const viskores::Vec<ComponentType, 3> nextPerpendicular =
      (perpendicular * static_cast<ComponentType>(this->CosStep)) +
      (viskores::Cross(axis, perpendicular) * static_cast<ComponentType>(this->SinStep));

    return nextPerpendicular - perpendicular;
  }

private:
  viskores::Vec3f_64 Axis;
  viskores::Vec3f_64 Center;
  viskores::Float64 CosStep;
  viskores::Float64 SinStep;
};

class ExtrusionRotationalCoordinates : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn flatPointId, WholeArrayIn baseCoordinates, FieldOut output);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  ExtrusionRotationalCoordinates(viskores::Vec3f_64 axis,
                                 viskores::Vec3f_64 center,
                                 viskores::FloatDefault sweepAngle,
                                 viskores::Id numberOfPlanes,
                                 viskores::Id numberOfPointsPerPlane,
                                 bool periodic)
    : Axis(axis)
    , Center(center)
    , SweepAngle(sweepAngle)
    , NumberOfPlanes(numberOfPlanes)
    , NumberOfPointsPerPlane(numberOfPointsPerPlane)
    , Periodic(periodic)
  {
  }

  template <typename CoordinatePortalType, typename OutputPointType>
  VISKORES_EXEC void operator()(viskores::Id flatPointId,
                                const CoordinatePortalType& baseCoordinates,
                                OutputPointType& output) const
  {
    using ComponentType = typename OutputPointType::ComponentType;

    const viskores::Id pointInPlane = flatPointId % this->NumberOfPointsPerPlane;
    const viskores::Id planeIndex = flatPointId / this->NumberOfPointsPerPlane;
    const ComponentType denominator = static_cast<ComponentType>(
      this->Periodic ? this->NumberOfPlanes : (this->NumberOfPlanes - 1));
    const ComponentType theta = static_cast<ComponentType>(this->SweepAngle) *
      static_cast<ComponentType>(planeIndex) / denominator;

    const viskores::Vec<ComponentType, 3> axis(static_cast<ComponentType>(this->Axis[0]),
                                               static_cast<ComponentType>(this->Axis[1]),
                                               static_cast<ComponentType>(this->Axis[2]));
    const viskores::Vec<ComponentType, 3> center(static_cast<ComponentType>(this->Center[0]),
                                                 static_cast<ComponentType>(this->Center[1]),
                                                 static_cast<ComponentType>(this->Center[2]));

    const viskores::Vec<ComponentType, 3> point = baseCoordinates.Get(pointInPlane);
    const viskores::Vec<ComponentType, 3> shifted = point - center;
    const viskores::Vec<ComponentType, 3> parallel = viskores::Dot(axis, shifted) * axis;
    const viskores::Vec<ComponentType, 3> perpendicular = shifted - parallel;
    const viskores::Vec<ComponentType, 3> rotated = center + parallel +
      (perpendicular * viskores::Cos(theta)) +
      (viskores::Cross(axis, perpendicular) * viskores::Sin(theta));

    output = rotated;
  }

private:
  viskores::Vec3f_64 Axis;
  viskores::Vec3f_64 Center;
  viskores::FloatDefault SweepAngle;
  viskores::Id NumberOfPlanes;
  viskores::Id NumberOfPointsPerPlane;
  bool Periodic;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_ExtrusionRotational_h
