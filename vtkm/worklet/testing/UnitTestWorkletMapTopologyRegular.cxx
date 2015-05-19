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
//  Copyright 2014. Los Alamos National Security
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

#include <vtkm/exec/arg/TopologyIdSet.h>
#include <vtkm/exec/arg/TopologyIdCount.h>
#include <vtkm/exec/arg/TopologyElementType.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

namespace test {

class MaxNodeOrCellValue : public vtkm::worklet::WorkletMapTopology
{
  static const int LEN_IDS = 4;
public:
  typedef void ControlSignature(FieldDestIn<Scalar> inCells,
                                FieldSrcIn<Scalar> inNodes,
                                TopologyIn<LEN_IDS> topology,
                                FieldDestOut<Scalar> outCells);
  //Todo: we need a way to mark what control signature item each execution signature for topology comes from
  typedef _4 ExecutionSignature(_1, _2,
                                vtkm::exec::arg::TopologyIdCount,
                                vtkm::exec::arg::TopologyElementType,
                                vtkm::exec::arg::TopologyIdSet);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  MaxNodeOrCellValue() { };

  VTKM_EXEC_EXPORT
  vtkm::Float32 operator()(const vtkm::Float32 &cellval,
                           const vtkm::Vec<vtkm::Float32,LEN_IDS> &nodevals,
                           const vtkm::Id &count,
                           const vtkm::Id &type,
                           const vtkm::Vec<vtkm::Id,LEN_IDS> &nodeIDs) const
  {
  //simple functor that returns the max of CellValue and nodeValue
  vtkm::Float32 max_value = cellval;
  for (vtkm::Id i=0; i<count; ++i)
      max_value = nodevals[i] > max_value ? nodevals[i] : max_value;
  return max_value;
  }

};


class AvgNodeToCellValue : public vtkm::worklet::WorkletMapTopology
{
  static const int LEN_IDS = 4;
public:
  typedef void ControlSignature(FieldSrcIn<Scalar> inNodes,
                                TopologyIn<LEN_IDS> topology,
                                FieldDestOut<Scalar> outCells);
  //Todo: we need a way to mark what control signature item each execution signature for topology comes from
  typedef _3 ExecutionSignature(_1,
                                vtkm::exec::arg::TopologyIdCount,
                                vtkm::exec::arg::TopologyElementType,
                                vtkm::exec::arg::TopologyIdSet);
  typedef _2 InputDomain;

  VTKM_CONT_EXPORT
  AvgNodeToCellValue() { };

  VTKM_EXEC_EXPORT
  vtkm::Float32 operator()(const vtkm::Vec<vtkm::Float32,LEN_IDS> &nodevals,
                           const vtkm::Id &count,
                           const vtkm::Id &type,
                           const vtkm::Vec<vtkm::Id,LEN_IDS> &nodeIDs) const
  {
      //simple functor that returns the average nodeValue.
      vtkm::Float32 avgVal = 0.0;
      for (vtkm::Id i=0; i<count; ++i)
          avgVal += nodevals[i];

      avgVal /= count;
      return avgVal;
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
    vtkm::cont::testing::MakeTestDataSet tds;
    vtkm::cont::DataSet *ds = tds.Make2DRegularDataSet0();
    vtkm::cont::CellSetStructured<2> *cs;
    cs = dynamic_cast<vtkm::cont::CellSetStructured<2> *>(ds->GetCellSet(0));
    VTKM_TEST_ASSERT(cs, "Structured cell set not found");
    
    //Run a worklet to populate a cell centered field.
    //Here, we're filling it with test values.
    vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
    ds->AddFieldViaCopy(outcellVals, 2);
    
    VTKM_TEST_ASSERT(test_equal(ds->GetNumberOfCellSets(), 1),
                     "Incorrect number of cell sets");
    
    VTKM_TEST_ASSERT(test_equal(ds->GetNumberOfFields(), 5),
                     "Incorrect number of fields");
    //Todo:
    //the scheduling should just be passed a CellSet, and not the
    //derived implementation. The vtkm::cont::CellSet should have
    //a method that return the nodesOfCellsConnectivity / structure
    //for that derived type. ( talk to robert for how dax did this )
    vtkm::worklet::DispatcherMapTopology< ::test::MaxNodeOrCellValue > dispatcher;
    dispatcher.Invoke(ds->GetField(3).GetData(),
                      ds->GetField(2).GetData(),
                      cs->GetNodeToCellConnectivity(),
                      ds->GetField(4).GetData());
    
    //make sure we got the right answer.
    vtkm::cont::ArrayHandle<vtkm::Float32> res;
    res = ds->GetField(4).GetData().CastToArrayHandle(vtkm::Float32(),
						      VTKM_DEFAULT_STORAGE_TAG());
    
    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(0), 100.1),
		     "Wrong result for MaxNodeOrCell worklet");
    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(1), 200.1),
		     "Wrong result for MaxNodeOrCell worklet");

    delete ds;
}

static void
TestAvgNodeToCell()
{
    std::cout<<"Testing AvgNodeToCell worklet"<<std::endl;
    vtkm::cont::testing::MakeTestDataSet tds;
    vtkm::cont::DataSet *ds = tds.Make2DRegularDataSet0();
    vtkm::cont::CellSetStructured<2> *cs;
    cs = dynamic_cast<vtkm::cont::CellSetStructured<2> *>(ds->GetCellSet(0));
    VTKM_TEST_ASSERT(cs, "Structured cell set not found");
    
    //Run a worklet to populate a cell centered field.
    //Here, we're filling it with test values.
    vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
    ds->AddFieldViaCopy(outcellVals, 2);
    
    VTKM_TEST_ASSERT(test_equal(ds->GetNumberOfCellSets(), 1),
                     "Incorrect number of cell sets");
    
    VTKM_TEST_ASSERT(test_equal(ds->GetNumberOfFields(), 5),
                     "Incorrect number of fields");
    //Todo:
    //the scheduling should just be passed a CellSet, and not the
    //derived implementation. The vtkm::cont::CellSet should have
    //a method that return the nodesOfCellsConnectivity / structure
    //for that derived type. ( talk to robert for how dax did this )
    vtkm::worklet::DispatcherMapTopology< ::test::AvgNodeToCellValue > dispatcher;
    dispatcher.Invoke(ds->GetField(2).GetData(),
                      cs->GetNodeToCellConnectivity(),
                      ds->GetField(4).GetData());
    
    //make sure we got the right answer.
    vtkm::cont::ArrayHandle<vtkm::Float32> res;
    res = ds->GetField(4).GetData().CastToArrayHandle(vtkm::Float32(),
						      VTKM_DEFAULT_STORAGE_TAG());
    
    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(0), 30.1),
		     "Wrong result for NodeToCellAverage worklet");
    VTKM_TEST_ASSERT(test_equal(res.GetPortalConstControl().Get(1), 40.1),
		     "Wrong result for NodeToCellAverage worklet");
    delete ds;
}

} // anonymous namespace

int UnitTestWorkletMapTopologyRegular(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestWorkletMapTopologyRegular);
}
