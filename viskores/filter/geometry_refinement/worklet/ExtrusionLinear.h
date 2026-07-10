//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_worklet_ExtrusionLinear_h
#define viskores_worklet_ExtrusionLinear_h

#include <viskores/Math.h>
#include <viskores/filter/geometry_refinement/worklet/Extrusion.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{

class ExtrusionLinearSweep
{
public:
  VISKORES_CONT
  explicit ExtrusionLinearSweep(viskores::Vec3f sweep)
    : Sweep(sweep)
  {
  }

  template <typename ComponentType>
  VISKORES_EXEC viskores::Vec<ComponentType, 3> operator()(
    const viskores::Vec<ComponentType, 3>&,
    const viskores::Vec<ComponentType, 3>&,
    const viskores::Vec<ComponentType, 3>&) const
  {
    return viskores::Vec<ComponentType, 3>(static_cast<ComponentType>(this->Sweep[0]),
                                           static_cast<ComponentType>(this->Sweep[1]),
                                           static_cast<ComponentType>(this->Sweep[2]));
  }

private:
  viskores::Vec3f Sweep;
};

class ExtrusionLinearCoordinates : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn flatPointId, WholeArrayIn baseCoordinates, FieldOut output);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  ExtrusionLinearCoordinates(viskores::Vec3f direction,
                             viskores::FloatDefault distance,
                             viskores::Id numberOfPlanes,
                             viskores::Id numberOfPointsPerPlane)
    : Direction(direction)
    , Distance(distance)
    , NumberOfPlanes(numberOfPlanes)
    , NumberOfPointsPerPlane(numberOfPointsPerPlane)
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
    const ComponentType denominator = static_cast<ComponentType>(this->NumberOfPlanes - 1);
    const ComponentType distance = static_cast<ComponentType>(this->Distance) *
      static_cast<ComponentType>(planeIndex) / denominator;

    const viskores::Vec<ComponentType, 3> direction(static_cast<ComponentType>(this->Direction[0]),
                                                    static_cast<ComponentType>(this->Direction[1]),
                                                    static_cast<ComponentType>(this->Direction[2]));
    const viskores::Vec<ComponentType, 3> point = baseCoordinates.Get(pointInPlane);

    output = point + (direction * distance);
  }

private:
  viskores::Vec3f Direction;
  viskores::FloatDefault Distance;
  viskores::Id NumberOfPlanes;
  viskores::Id NumberOfPointsPerPlane;
};
}
} // namespace viskores::worklet

#endif // viskores_worklet_ExtrusionLinear_h
