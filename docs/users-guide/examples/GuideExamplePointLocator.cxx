//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/PointLocatorSparseGrid.h>

#include <viskores/Math.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id DimensionSize = 50;
const viskores::Id3 DimensionSizes = viskores::Id3(DimensionSize);

////
//// BEGIN-EXAMPLE UsePointLocator
////
/// Worklet that generates for each input coordinate a unit vector that points
/// to the closest point in a locator.
struct PointToClosestWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, ExecObject, WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template<typename Point,
           typename PointLocatorExecObject,
           typename CoordinateSystemPortal,
           typename OutType>
  VISKORES_EXEC void operator()(const Point& queryPoint,
                                const PointLocatorExecObject& pointLocator,
                                const CoordinateSystemPortal& coordinateSystem,
                                OutType& out) const
  {
    // Use the point locator to find the point in the locator closest to the point
    // given.
    viskores::Id pointId;
    viskores::FloatDefault distanceSquared;
    pointLocator.FindNearestNeighbor(queryPoint, pointId, distanceSquared);

    // Use this information to find the nearest point and create a unit vector
    // pointing to it.
    if (pointId >= 0)
    {
      // Get nearest point coordinate.
      auto point = coordinateSystem.Get(pointId);

      // Get the vector pointing to this point
      out = point - queryPoint;

      // Convert to unit vector (if possible)
      if (distanceSquared > viskores::Epsilon<viskores::FloatDefault>())
      {
        out = viskores::RSqrt(distanceSquared) * out;
      }
    }
    else
    {
      this->RaiseError("Locator could not find closest point.");
    }
  }
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoQueryPoints
{
  viskores::cont::Invoker Invoke;

  viskores::cont::ArrayHandle<viskores::Vec3f> QueryPoints;

  VISKORES_CONT viskores::cont::ArrayHandle<viskores::Vec3f> Run(
    const viskores::cont::DataSet& inDataSet)
  {
    // Note: when more point locators are created, we might want to switch the
    // example to a different (perhaps more general) one.
    //// RESUME-EXAMPLE
    ////
    //// BEGIN-EXAMPLE ConstructPointLocator
    ////
    viskores::cont::PointLocatorSparseGrid pointLocator;
    pointLocator.SetCoordinates(inDataSet.GetCoordinateSystem());
    pointLocator.Update();
    ////
    //// END-EXAMPLE ConstructPointLocator
    ////

    viskores::cont::ArrayHandle<viskores::Vec3f> pointDirections;

    this->Invoke(PointToClosestWorklet{},
                 this->QueryPoints,
                 &pointLocator,
                 pointLocator.GetCoordinates(),
                 pointDirections);
    ////
    //// END-EXAMPLE UsePointLocator
    ////

    return pointDirections;
  }
};

void TestPointLocator()
{
  using ValueType = viskores::Vec3f;
  using ArrayType = viskores::cont::ArrayHandle<ValueType>;

  viskores::cont::DataSet data =
    viskores::cont::DataSetBuilderUniform::Create(DimensionSizes);

  DemoQueryPoints demo;

  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleUniformPointCoordinates(
                              DimensionSizes - viskores::Id3(1), ValueType(0.75f)),
                            demo.QueryPoints);

  ArrayType pointers = demo.Run(data);

  auto expected = viskores::cont::make_ArrayHandleConstant(
    viskores::Vec3f(0.57735f), demo.QueryPoints.GetNumberOfValues());

  std::cout << "Expected: ";
  viskores::cont::printSummary_ArrayHandle(expected, std::cout);

  std::cout << "Calculated: ";
  viskores::cont::printSummary_ArrayHandle(pointers, std::cout);

  VISKORES_TEST_ASSERT(test_equal_portals(expected.ReadPortal(), pointers.ReadPortal()));
}

void Run()
{
  TestPointLocator();
}

} // anonymous namespace

int GuideExamplePointLocator(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
