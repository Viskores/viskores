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

namespace test_explicit {

class MaxPointOrCellValue : public vtkm::worklet::WorkletMapTopology
{
public:
  typedef void ControlSignature(FieldInTo<Scalar> inCells,
                                FieldInFrom<Scalar> inPoints,
                                TopologyIn topology,
                                FieldOut<Scalar> outCells);
  typedef void ExecutionSignature(_1, _4, _2, FromCount, CellShape, FromIndices);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  MaxPointOrCellValue() { }

  template<typename InCellType,
           typename OutCellType,
           typename InPointVecType,
           typename FromIndexType>
  VTKM_EXEC_EXPORT
  void operator()(const InCellType &cellValue,
                  OutCellType &maxValue,
                  const InPointVecType &pointValues,
                  const vtkm::IdComponent &numPoints,
                  const vtkm::Id &vtkmNotUsed(type),
                  const FromIndexType &vtkmNotUsed(pointIDs)) const
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

class AveragePointToCellValue : public vtkm::worklet::WorkletMapTopology
{
public:
  typedef void ControlSignature(FieldInFrom<Scalar> inPoints,
                                TopologyIn topology,
                                FieldOut<Scalar> outCells);
  typedef void ExecutionSignature(_1, _3, FromCount);
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

}

namespace {

static void TestMaxPointOrCell();
static void TestAvgPointToCell();

void TestWorkletMapTopologyExplicit()
{
  typedef vtkm::cont::internal::DeviceAdapterTraits<
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
  std::cout << "Testing Topology Worklet ( Explicit ) on device adapter: "
            << DeviceAdapterTraits::GetId() << std::endl;

    TestMaxPointOrCell();
    TestAvgPointToCell();
}


static void
TestMaxPointOrCell()
{
  std::cout<<"Testing MaxPointOfCell worklet"<<std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet1();

  //Run a worklet to populate a cell centered field.
  //Here, we're filling it with test values.
  vtkm::cont::Field f("outcellvar",
                      1,
                      vtkm::cont::Field::ASSOC_CELL_SET,
                      std::string("cells"),
                      vtkm::Float32());

  dataSet.AddField(f);

  VTKM_TEST_ASSERT(dataSet.GetNumberOfCellSets() == 1,
                   "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == 3,
                   "Incorrect number of fields");

  vtkm::worklet::DispatcherMapTopology< ::test_explicit::MaxPointOrCellValue >
      dispatcher;
  dispatcher.Invoke(dataSet.GetField("cellvar").GetData(),
                    dataSet.GetField("pointvar").GetData(),
                    dataSet.GetCellSet(0),
                    dataSet.GetField("outcellvar").GetData());

  //Make sure we got the right answer.
  vtkm::cont::ArrayHandle<vtkm::Float32> res;
  res = dataSet.GetField("outcellvar").GetData().
      CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
  VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(0), 100.1f),
                   "Wrong result for PointToCellMax worklet");
  VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(1), 100.2f),
                   "Wrong result for PointToCellMax worklet");
}

static void
TestAvgPointToCell()
{
  std::cout<<"Testing AvgPointToCell worklet"<<std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DExplicitDataSet1();

  //Run a worklet to populate a cell centered field.
  //Here, we're filling it with test values.
  vtkm::cont::Field f("outcellvar",
                      1,
                      vtkm::cont::Field::ASSOC_CELL_SET,
                      std::string("cells"),
                      vtkm::Float32());

  dataSet.AddField(f);

  VTKM_TEST_ASSERT(dataSet.GetNumberOfCellSets() == 1,
                       "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == 3,
                       "Incorrect number of fields");

  vtkm::worklet::DispatcherMapTopology< ::test_explicit::AveragePointToCellValue > dispatcher;
  dispatcher.Invoke(dataSet.GetField("pointvar").GetData(),
                    dataSet.GetCellSet(),
                    dataSet.GetField("outcellvar").GetData());

  //make sure we got the right answer.
  vtkm::cont::ArrayHandle<vtkm::Float32> res;
  res = dataSet.GetField("outcellvar").GetData().CastToArrayHandle(vtkm::Float32(),
                                                    VTKM_DEFAULT_STORAGE_TAG());

  VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(0), 20.1333f),
                   "Wrong result for PointToCellAverage worklet");
  VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(1), 35.2f),
                   "Wrong result for PointToCellAverage worklet");
}

} // anonymous namespace

int UnitTestWorkletMapTopologyExplicit(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestWorkletMapTopologyExplicit);
}
