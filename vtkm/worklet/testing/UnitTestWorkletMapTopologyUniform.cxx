//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/Math.h>

#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace test_uniform {

class MaxPointOrCellValue : public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature(FieldInCell<Scalar> inCells,
                                FieldInPoint<Scalar> inPoints,
                                TopologyIn topology,
                                FieldOutCell<Scalar> outCells);
  typedef void ExecutionSignature(_1, _4, _2, PointCount, CellShape, PointIndices);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  MaxPointOrCellValue() { }

  template<typename InCellType,
           typename OutCellType,
           typename InPointVecType,
           typename CellShapeTag,
           typename PointIndexType>
  VTKM_EXEC_EXPORT
  void operator()(const InCellType &cellValue,
                  OutCellType &maxValue,
                  const InPointVecType &pointValues,
                  const vtkm::IdComponent &numPoints,
                  const CellShapeTag &vtkmNotUsed(type),
                  const PointIndexType &vtkmNotUsed(pointIDs)) const
  {
    //simple functor that returns the max of cellValue and pointValue
    maxValue = static_cast<OutCellType>(cellValue);
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      maxValue = vtkm::Max(maxValue,
                           static_cast<OutCellType>(pointValues[pointIndex]));
    }
  }
};

class AveragePointToCellValue : public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature(FieldInPoint<Scalar> inPoints,
                                TopologyIn topology,
                                FieldOutCell<Scalar> outCells);
  typedef void ExecutionSignature(_1, _3, PointCount);
  typedef _2 InputDomain;

  VTKM_CONT_EXPORT
  AveragePointToCellValue() { }

  template<typename PointVecType, typename OutType>
  VTKM_EXEC_EXPORT
  void operator()(const PointVecType &pointValues,
                  OutType &avgVal,
                  const vtkm::IdComponent &numPoints) const
  {
    //simple functor that returns the average pointValue.
    avgVal = static_cast<OutType>(pointValues[0]);
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; ++pointIndex)
    {
      avgVal += static_cast<OutType>(pointValues[pointIndex]);
    }
    avgVal = avgVal / static_cast<OutType>(numPoints);
  }
};

class AverageCellToPointValue : public vtkm::worklet::WorkletMapCellToPoint
{
public:
  typedef void ControlSignature(FieldInCell<Scalar> inCells,
                                TopologyIn topology,
                                FieldOut<Scalar> outPoints);
  typedef void ExecutionSignature(_1, _3, CellCount);
  typedef _2 InputDomain;

  VTKM_CONT_EXPORT
  AverageCellToPointValue() { }

  template<typename CellVecType, typename OutType>
  VTKM_EXEC_EXPORT
  void operator()(const CellVecType &cellValues,
                  OutType &avgVal,
                  const vtkm::IdComponent &numCellIDs) const
  {
    //simple functor that returns the average cell Value.
    avgVal = static_cast<OutType>(cellValues[0]);
    for (vtkm::IdComponent cellIndex = 1; cellIndex < numCellIDs; ++cellIndex)
    {
      avgVal += static_cast<OutType>(cellValues[cellIndex]);
    }
    avgVal = avgVal / static_cast<OutType>(numCellIDs);
  }
};

struct CheckStructuredUniformPointCoords
    : public vtkm::worklet::WorkletMapPointToCell
{
  typedef void ControlSignature(TopologyIn topology,
                                FieldInPoint<Vec3> pointCoords);
  typedef void ExecutionSignature(_2);

  VTKM_CONT_EXPORT
  CheckStructuredUniformPointCoords() {  }

  template<vtkm::IdComponent NumDimensions>
  VTKM_EXEC_EXPORT
  void operator()(
      const vtkm::VecRectilinearPointCoordinates<NumDimensions> &
        vtkmNotUsed(coords)) const
  {
    // Success if here.
  }

  template<typename PointCoordsVecType>
  VTKM_EXEC_EXPORT
  void operator()(const PointCoordsVecType &vtkmNotUsed(coords)) const
  {
    this->RaiseError("Got wrong point coordinates type.");
  }
};

}

namespace {

static void TestMaxPointOrCell();
static void TestAvgPointToCell();
static void TestAvgCellToPoint();
static void TestStructuredUniformPointCoords();

void TestWorkletMapTopologyUniform()
{
    typedef vtkm::cont::DeviceAdapterTraits<
        VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
    std::cout << "Testing Topology Worklet ( Uniform ) on device adapter: "
              << DeviceAdapterTraits::GetName() << std::endl;

    TestMaxPointOrCell();
    TestAvgPointToCell();
    TestAvgCellToPoint();
    TestStructuredUniformPointCoords();
}

static void
TestMaxPointOrCell()
{
  std::cout<<"Testing MaxPointOfCell worklet"<<std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology< ::test_uniform::MaxPointOrCellValue > dispatcher;
  dispatcher.Invoke(dataSet.GetField("cellvar").GetData(),
                    dataSet.GetField("pointvar").GetData(),
                    // We know that the cell set is a structured 2D grid and
                    // The worklet does not work with general types because
                    // of the way we get cell indices. We need to make that
                    // part more flexible.
                    dataSet.GetCellSet(0).ResetCellSetList(
                      vtkm::cont::CellSetListTagStructured2D()),
                    result);

  //make sure we got the right answer.

  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for MaxPointOrCell worklet");
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 200.1f),
                   "Wrong result for MaxPointOrCell worklet");
}

static void
TestAvgPointToCell()
{
  std::cout<<"Testing AvgPointToCell worklet"<<std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology< ::test_uniform::AveragePointToCellValue > dispatcher;
  dispatcher.Invoke(dataSet.GetField("pointvar").GetData(),
                    // We know that the cell set is a structured 2D grid and
                    // The worklet does not work with general types because
                    // of the way we get cell indices. We need to make that
                    // part more flexible.
                    dataSet.GetCellSet(0).ResetCellSetList(
                      vtkm::cont::CellSetListTagStructured2D()),
                    result);

  //make sure we got the right answer.
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 30.1f),
                   "Wrong result for PointToCellAverage worklet");
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 40.1f),
                   "Wrong result for PointToCellAverage worklet");
}

static void
TestAvgCellToPoint()
{
  std::cout<<"Testing AvgCellToPoint worklet"<<std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  vtkm::worklet::DispatcherMapTopology< ::test_uniform::AverageCellToPointValue > dispatcher;
  dispatcher.Invoke(dataSet.GetField("cellvar").GetData(),
                    // We know that the cell set is a structured 2D grid and
                    // The worklet does not work with general types because
                    // of the way we get cell indices. We need to make that
                    // part more flexible.
                    dataSet.GetCellSet(0).ResetCellSetList(
                      vtkm::cont::CellSetListTagStructured2D()),
                    result);

  //make sure we got the right answer.
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for CellToPointAverage worklet");
  VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(1), 150.1f),
                   "Wrong result for CellToPointAverage worklet");
}

static void
TestStructuredUniformPointCoords()
{
  std::cout << "Testing uniform point coordinates in structured grids"
            << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::worklet::DispatcherMapTopology<
      ::test_uniform::CheckStructuredUniformPointCoords> dispatcher;

  vtkm::cont::DataSet dataSet3D = testDataSet.Make3DUniformDataSet0();
  dispatcher.Invoke(dataSet3D.GetCellSet(),
                    dataSet3D.GetCoordinateSystem().GetData());

  vtkm::cont::DataSet dataSet2D = testDataSet.Make2DUniformDataSet0();
  dispatcher.Invoke(dataSet2D.GetCellSet(),
                    dataSet2D.GetCoordinateSystem().GetData());
}

} // anonymous namespace

int UnitTestWorkletMapTopologyUniform(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestWorkletMapTopologyUniform);
}
