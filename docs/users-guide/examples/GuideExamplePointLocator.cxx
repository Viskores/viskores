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

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/PointLocatorSparseGrid.h>

#include <vtkm/Math.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id DimensionSize = 50;
const vtkm::Id3 DimensionSizes = vtkm::Id3(DimensionSize);

////
//// BEGIN-EXAMPLE UsePointLocator
////
/// Worklet that generates for each input coordinate a unit vector that points
/// to the closest point in a locator.
struct PointToClosestWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, ExecObject, WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template<typename Point,
           typename PointLocatorExecObject,
           typename CoordinateSystemPortal,
           typename OutType>
  VTKM_EXEC void operator()(const Point& queryPoint,
                            const PointLocatorExecObject& pointLocator,
                            const CoordinateSystemPortal& coordinateSystem,
                            OutType& out) const
  {
    // Use the point locator to find the point in the locator closest to the point
    // given.
    vtkm::Id pointId;
    vtkm::FloatDefault distanceSquared;
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
      if (distanceSquared > vtkm::Epsilon<vtkm::FloatDefault>())
      {
        out = vtkm::RSqrt(distanceSquared) * out;
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
  vtkm::cont::Invoker Invoke;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> QueryPoints;

  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Vec3f> Run(
    const vtkm::cont::DataSet& inDataSet)
  {
    // Note: when more point locators are created, we might want to switch the
    // example to a different (perhaps more general) one.
    //// RESUME-EXAMPLE
    ////
    //// BEGIN-EXAMPLE ConstructPointLocator
    ////
    vtkm::cont::PointLocatorSparseGrid pointLocator;
    pointLocator.SetCoordinates(inDataSet.GetCoordinateSystem());
    pointLocator.Update();
    ////
    //// END-EXAMPLE ConstructPointLocator
    ////

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pointDirections;

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
  using ValueType = vtkm::Vec3f;
  using ArrayType = vtkm::cont::ArrayHandle<ValueType>;

  vtkm::cont::DataSet data = vtkm::cont::DataSetBuilderUniform::Create(DimensionSizes);

  DemoQueryPoints demo;

  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleUniformPointCoordinates(
                          DimensionSizes - vtkm::Id3(1), ValueType(0.75f)),
                        demo.QueryPoints);

  ArrayType pointers = demo.Run(data);

  auto expected = vtkm::cont::make_ArrayHandleConstant(
    vtkm::Vec3f(0.57735f), demo.QueryPoints.GetNumberOfValues());

  std::cout << "Expected: ";
  vtkm::cont::printSummary_ArrayHandle(expected, std::cout);

  std::cout << "Calculated: ";
  vtkm::cont::printSummary_ArrayHandle(pointers, std::cout);

  VTKM_TEST_ASSERT(test_equal_portals(expected.ReadPortal(), pointers.ReadPortal()));
}

void Run()
{
  TestPointLocator();
}

} // anonymous namespace

int GuideExamplePointLocator(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
