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

#include <vtkm/cont/DataSet.h>

#include <vtkm/exec/TopologyData.h>
#include <vtkm/exec/arg/TopologyIdSet.h>
#include <vtkm/exec/arg/TopologyIdCount.h>
#include <vtkm/exec/arg/TopologyElementType.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace test_regular {

class MaxNodeOrCellValue : public vtkm::worklet::WorkletMapTopology
{
  static const int LEN_IDS = 4;
public:
  typedef void ControlSignature(FieldDestIn<Scalar> inCells,
                                FieldSrcIn<Scalar> inNodes,
                                TopologyIn<LEN_IDS> topology,
                                FieldDestOut<Scalar> outCells);
  //Todo: we need a way to mark what control signature item each execution signature for topology comes from
  typedef void ExecutionSignature(_1, _4, _2,
                                vtkm::exec::arg::TopologyIdCount,
                                vtkm::exec::arg::TopologyElementType,
                                vtkm::exec::arg::TopologyIdSet);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  MaxNodeOrCellValue() { };

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(const T &cellval,
                  T& max_value,
                  const vtkm::exec::TopologyData<T,LEN_IDS> &nodevals,
                  const vtkm::Id &count,
                  const vtkm::Id & vtkmNotUsed(type),
                  const vtkm::exec::TopologyData<vtkm::Id,LEN_IDS> & vtkmNotUsed(nodeIDs)) const
  {
  //simple functor that returns the max of CellValue and nodeValue
  max_value = cellval;
  for (vtkm::IdComponent i=0; i< count; ++i)
    {
    max_value = nodevals[i] > max_value ? nodevals[i] : max_value;
    }
  }

  template<typename T1, typename T2, typename T3>
  VTKM_EXEC_EXPORT
  void operator()(const T1 &,
                  T2 &,
                  const vtkm::exec::TopologyData<T3,LEN_IDS> &,
                  const vtkm::Id &,
                  const vtkm::Id &,
                  const vtkm::exec::TopologyData<vtkm::Id,LEN_IDS> &) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
  }

};

class AvgNodeToCellValue : public vtkm::worklet::WorkletMapTopology
{
  static const int LEN_IDS = 4;
public:
  typedef void ControlSignature(FieldSrcIn<Scalar> inNodes,
                                TopologyIn<LEN_IDS> topology,
                                FieldDestOut<Scalar> outCells);
  typedef void ExecutionSignature(_1,
                                  _3,
                                  vtkm::exec::arg::TopologyIdCount);
  typedef _2 InputDomain;

  VTKM_CONT_EXPORT
  AvgNodeToCellValue() { };

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::exec::TopologyData<T,LEN_IDS> &nodevals,
                  T& avgVal,
                  const vtkm::Id &count) const
  {
    //simple functor that returns the average nodeValue.
    avgVal = nodevals[0];
    for (vtkm::IdComponent i=1; i<count; ++i)
    {
      avgVal += nodevals[i];
    }
    avgVal = avgVal / count;
  }

  template<typename T1, typename T2>
  VTKM_EXEC_EXPORT
  void operator()(const T1 &, T2 &, const vtkm::Id &) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
  }

};

}

namespace {

static void TestMaxNodeOrCell();
static void TestAvgNodeToCell();

void TestWorkletMapTopologyRegular()
{
    typedef vtkm::cont::internal::DeviceAdapterTraits<
        VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
    std::cout << "Testing Topology Worklet ( Regular ) on device adapter: "
              << DeviceAdapterTraits::GetId() << std::endl;

    TestMaxNodeOrCell();
    TestAvgNodeToCell();
}

static void
TestMaxNodeOrCell()
{
    std::cout<<"Testing MaxNodeOfCell worklet"<<std::endl;
    vtkm::cont::testing::MakeTestDataSet testDataSet;
    vtkm::cont::DataSet dataSet = testDataSet.Make2DRegularDataSet0();

    //Run a worklet to populate a cell centered field.
    //Here, we're filling it with test values.
    vtkm::cont::Field f("outcellvar",
                      1,
                      vtkm::cont::Field::ASSOC_CELL_SET,
                      std::string("cells"),
                      vtkm::Float32());

    dataSet.AddField(f);

    VTKM_TEST_ASSERT(test_equal(dataSet.GetNumberOfCellSets(), 1),
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(test_equal(dataSet.GetNumberOfFields(), 5),
                     "Incorrect number of fields");
    vtkm::worklet::DispatcherMapTopology< ::test_regular::MaxNodeOrCellValue > dispatcher;
    dispatcher.Invoke(dataSet.GetField("cellvar").GetData(),
                      dataSet.GetField("nodevar").GetData(),
                      // We know that the cell set is a structured 2D grid and
                      // The worklet does not work with general types because
                      // of the way we get cell indices. We need to make that
                      // part more flexible.
                      dataSet.GetCellSet(0).ResetCellSetList(
                        vtkm::cont::CellSetListTagStructured2D()),
                      dataSet.GetField(4).GetData());

    //make sure we got the right answer.
    vtkm::cont::ArrayHandle<vtkm::Float32> res;
    res = dataSet.GetField(4).GetData().CastToArrayHandle(vtkm::Float32(),
                                                      VTKM_DEFAULT_STORAGE_TAG());

    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(0), 100.1f),
                     "Wrong result for MaxNodeOrCell worklet");
    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(1), 200.1f),
                     "Wrong result for MaxNodeOrCell worklet");
}

static void
TestAvgNodeToCell()
{
    std::cout<<"Testing AvgNodeToCell worklet"<<std::endl;
    vtkm::cont::testing::MakeTestDataSet testDataSet;
    vtkm::cont::DataSet dataSet = testDataSet.Make2DRegularDataSet0();

    //Run a worklet to populate a cell centered field.
    //Here, we're filling it with test values.
    vtkm::cont::Field f("outcellvar",
                      1,
                      vtkm::cont::Field::ASSOC_CELL_SET,
                      std::string("cells"),
                      vtkm::Float32());

    dataSet.AddField(f);

    VTKM_TEST_ASSERT(test_equal(dataSet.GetNumberOfCellSets(), 1),
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(test_equal(dataSet.GetNumberOfFields(), 5),
                     "Incorrect number of fields");
    vtkm::worklet::DispatcherMapTopology< ::test_regular::AvgNodeToCellValue > dispatcher;
    dispatcher.Invoke(dataSet.GetField("nodevar").GetData(),
                      // We know that the cell set is a structured 2D grid and
                      // The worklet does not work with general types because
                      // of the way we get cell indices. We need to make that
                      // part more flexible.
                      dataSet.GetCellSet(0).ResetCellSetList(
                        vtkm::cont::CellSetListTagStructured2D()),
                      dataSet.GetField("outcellvar").GetData());

    //make sure we got the right answer.
    vtkm::cont::ArrayHandle<vtkm::Float32> res;
    res = dataSet.GetField(4).GetData().CastToArrayHandle(vtkm::Float32(),
                                                      VTKM_DEFAULT_STORAGE_TAG());

    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(0), 30.1f),
                     "Wrong result for NodeToCellAverage worklet");
    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(1), 40.1f),
                     "Wrong result for NodeToCellAverage worklet");
}

} // anonymous namespace

int UnitTestWorkletMapTopologyRegular(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestWorkletMapTopologyRegular);
}
