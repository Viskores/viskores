//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_worklet_Extrusion_h
#define viskores_worklet_Extrusion_h

#include <viskores/Math.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{

struct ExtrusionOutputToInputPlane
{
  viskores::Id NumberOfIdsPerPlane = 0;

  VISKORES_EXEC_CONT
  viskores::Id operator()(viskores::Id outputId) const
  {
    return outputId % this->NumberOfIdsPerPlane;
  }
};

template <typename SweepFunctor>
class OrientExtrudedTriangleConnectivity : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn baseConnectivity,
                                WholeArrayIn baseCoordinates,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  explicit OrientExtrudedTriangleConnectivity(SweepFunctor sweep)
    : Sweep(sweep)
  {
  }

  template <typename ConnectivityType, typename CoordinatePortalType>
  VISKORES_EXEC void operator()(const ConnectivityType& baseConnectivity,
                                const CoordinatePortalType& baseCoordinates,
                                viskores::Id3& output) const
  {
    using CoordinateType = typename CoordinatePortalType::ValueType;
    using ComponentType = typename CoordinateType::ComponentType;

    const viskores::Id pointId0 = baseConnectivity[0];
    const viskores::Id pointId1 = baseConnectivity[1];
    const viskores::Id pointId2 = baseConnectivity[2];

    const viskores::Vec<ComponentType, 3> point0 = baseCoordinates.Get(pointId0);
    const viskores::Vec<ComponentType, 3> point1 = baseCoordinates.Get(pointId1);
    const viskores::Vec<ComponentType, 3> point2 = baseCoordinates.Get(pointId2);

    const viskores::Vec<ComponentType, 3> normal =
      viskores::Cross(point1 - point0, point2 - point0);
    const viskores::Vec<ComponentType, 3> sweep = this->Sweep(point0, point1, point2);
    const bool flip = viskores::Dot(normal, sweep) < ComponentType{ 0 };

    output[0] = pointId0;
    output[1] = flip ? pointId2 : pointId1;
    output[2] = flip ? pointId1 : pointId2;
  }

private:
  SweepFunctor Sweep;
};

// The extrusion filters generate each plane with the same point ordering, so
// explicit output uses identity point correspondence between adjacent planes.
class ExtrudedCellSetToExplicitWedgeConnectivity : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn flatConnectivityId,
                                WholeArrayIn baseConnectivity,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3);

  VISKORES_CONT
  ExtrudedCellSetToExplicitWedgeConnectivity(viskores::Id numberOfCellsPerPlane,
                                             viskores::Id numberOfPointsPerPlane,
                                             viskores::Id numberOfPlanes)
    : NumberOfCellsPerPlane(numberOfCellsPerPlane)
    , NumberOfPointsPerPlane(numberOfPointsPerPlane)
    , NumberOfPlanes(numberOfPlanes)
  {
  }

  template <typename ConnectivityPortalType>
  VISKORES_EXEC void operator()(viskores::Id flatConnectivityId,
                                const ConnectivityPortalType& baseConnectivity,
                                viskores::Id& output) const
  {
    const viskores::Id cellId = flatConnectivityId / 6;
    const viskores::Id localPointId = flatConnectivityId % 6;
    const viskores::Id triangleId = cellId % this->NumberOfCellsPerPlane;
    const viskores::Id plane0 = cellId / this->NumberOfCellsPerPlane;
    const viskores::Id plane1 = (plane0 < (this->NumberOfPlanes - 1)) ? (plane0 + 1) : 0;
    const viskores::Id basePointId = baseConnectivity.Get((triangleId * 3) + (localPointId % 3));

    output = (localPointId < 3) ? ((plane0 * this->NumberOfPointsPerPlane) + basePointId)
                                : ((plane1 * this->NumberOfPointsPerPlane) + basePointId);
  }

private:
  viskores::Id NumberOfCellsPerPlane;
  viskores::Id NumberOfPointsPerPlane;
  viskores::Id NumberOfPlanes;
};

}
} // namespace viskores::worklet

#endif // viskores_worklet_Extrusion_h
