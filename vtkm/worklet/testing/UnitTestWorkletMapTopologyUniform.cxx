//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/CellAverage.h>
#include <vtkm/worklet/PointAverage.h>

#include <vtkm/Math.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterTag.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace test_uniform
{

class MaxPointOrCellValue : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(FieldInCell inCells,
                                FieldInPoint inPoints,
                                CellSetIn topology,
                                FieldOutCell outCells);
  using ExecutionSignature = void(_1, _4, _2, PointCount, CellShape, PointIndices);
  using InputDomain = _3;

  VTKM_CONT
  MaxPointOrCellValue() {}

  template <typename InCellType,
            typename OutCellType,
            typename InPointVecType,
            typename CellShapeTag,
            typename PointIndexType>
  VTKM_EXEC void operator()(const InCellType& cellValue,
                            OutCellType& maxValue,
                            const InPointVecType& pointValues,
                            const vtkm::IdComponent& numPoints,
                            const CellShapeTag& vtkmNotUsed(type),
                            const PointIndexType& vtkmNotUsed(pointIDs)) const
  {
    //simple functor that returns the max of cellValue and pointValue
    maxValue = static_cast<OutCellType>(cellValue);
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      maxValue = vtkm::Max(maxValue, static_cast<OutCellType>(pointValues[pointIndex]));
    }
  }
};

struct CheckStructuredUniformPointCoords : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn topology, FieldInPoint pointCoords);
  using ExecutionSignature = void(_2);

  VTKM_CONT
  CheckStructuredUniformPointCoords() {}

  template <vtkm::IdComponent NumDimensions>
  VTKM_EXEC void operator()(
    const vtkm::VecAxisAlignedPointCoordinates<NumDimensions>& vtkmNotUsed(coords)) const
  {
    // Success if here.
  }

  template <typename PointCoordsVecType>
  VTKM_EXEC void operator()(const PointCoordsVecType& vtkmNotUsed(coords)) const
  {
    this->RaiseError("Got wrong point coordinates type.");
  }
};
}

namespace
{

static void TestMaxPointOrCell();
static void TestAvgPointToCell();
static void TestAvgCellToPoint();
static void TestStructuredUniformPointCoords();

void TestWorkletMapTopologyUniform(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Topology Worklet ( Uniform ) on device adapter: " << id.GetName()
            << std::endl;

  TestMaxPointOrCell();
  TestAvgPointToCell();
  TestAvgCellToPoint();
  TestStructuredUniformPointCoords();
}

static void TestMaxPointOrCell()
{
  std::cout << "Testing MaxPointOfCell worklet" << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology<::test_uniform::MaxPointOrCellValue> dispatcher;
  dispatcher.Invoke(dataSet.GetField("cellvar").GetData().ResetTypes(vtkm::TypeListFieldScalar()),
                    dataSet.GetField("pointvar").GetData().ResetTypes(vtkm::TypeListFieldScalar()),
                    // We know that the cell set is a structured 2D grid and
                    // The worklet does not work with general types because
                    // of the way we get cell indices. We need to make that
                    // part more flexible.
                    dataSet.GetCellSet().ResetCellSetList(vtkm::cont::CellSetListStructured2D()),
                    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for MaxPointOrCell worklet");
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 200.1f),
                   "Wrong result for MaxPointOrCell worklet");
}

static void TestAvgPointToCell()
{
  std::cout << "Testing AvgPointToCell worklet" << std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  auto cellset = dataSet.GetCellSet().ResetCellSetList(vtkm::cont::CellSetListStructured2D());

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage> dispatcher;
  dispatcher.Invoke(
    // We know that the cell set is a structured 2D grid and
    // The worklet does not work with general types because
    // of the way we get cell indices. We need to make that
    // part more flexible.
    &cellset,
    dataSet.GetField("pointvar"),
    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 30.1f),
                   "Wrong result for PointToCellAverage worklet");
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 40.1f),
                   "Wrong result for PointToCellAverage worklet");

  std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
  bool exceptionThrown = false;
  try
  {
    dispatcher.Invoke( // We know that the cell set is a structured 2D grid and
      // The worklet does not work with general types because
      // of the way we get cell indices. We need to make that
      // part more flexible.
      dataSet.GetCellSet().ResetCellSetList(vtkm::cont::CellSetListStructured2D()),
      dataSet.GetField("cellvar"), // should be pointvar
      result);
  }
  catch (vtkm::cont::ErrorBadValue& error)
  {
    std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
    exceptionThrown = true;
  }
  VTKM_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
}

static void TestAvgCellToPoint()
{
  std::cout << "Testing AvgCellToPoint worklet" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::PointAverage> dispatcher;
  dispatcher.Invoke(
    // We know that the cell set is a structured 2D grid and
    // The worklet does not work with general types because
    // of the way we get cell indices. We need to make that
    // part more flexible.
    dataSet.GetCellSet().ResetCellSetList(vtkm::cont::CellSetListStructured2D()),
    dataSet.GetField("cellvar"),
    result);

  std::cout << "Make sure we got the right answer." << std::endl;
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for CellToPointAverage worklet");
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 150.1f),
                   "Wrong result for CellToPointAverage worklet");

  std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
  bool exceptionThrown = false;
  try
  {
    dispatcher.Invoke( // We know that the cell set is a structured 2D grid and
      // The worklet does not work with general types because
      // of the way we get cell indices. We need to make that
      // part more flexible.
      dataSet.GetCellSet().ResetCellSetList(vtkm::cont::CellSetListStructured2D()),
      dataSet.GetField("pointvar"), // should be cellvar
      result);
  }
  catch (vtkm::cont::ErrorBadValue& error)
  {
    std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
    exceptionThrown = true;
  }
  VTKM_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
}

static void TestStructuredUniformPointCoords()
{
  std::cout << "Testing uniform point coordinates in structured grids" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherMapTopology<::test_uniform::CheckStructuredUniformPointCoords>
    dispatcher;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(), dataSet3D.GetCoordinateSystem());

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(), dataSet2D.GetCoordinateSystem());
}

} // anonymous namespace

int UnitTestWorkletMapTopologyUniform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(TestWorkletMapTopologyUniform, argc, argv);
}
