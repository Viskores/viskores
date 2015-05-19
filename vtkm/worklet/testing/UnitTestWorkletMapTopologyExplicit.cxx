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

class CellValue : public vtkm::worklet::WorkletMapTopology
{
  static const int LEN_IDS = 6;
public:
  typedef void ControlSignature(FieldDestIn<Scalar> inCells, FieldSrcIn<Scalar> inNodes, TopologyIn<LEN_IDS> topology, FieldDestOut<Scalar> outCells);
  //Todo: we need a way to mark what control signature item each execution signature for topology comes from
  typedef _4 ExecutionSignature(_1, _2, vtkm::exec::arg::TopologyIdCount, vtkm::exec::arg::TopologyElementType, vtkm::exec::arg::TopologyIdSet);
  typedef _3 InputDomain;

  VTKM_CONT_EXPORT
  CellValue() { };

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
    {
    max_value = nodevals[i] > max_value ? nodevals[i] : max_value;
    }
  return max_value;
  }

};

}

namespace {

void TestWorkletMapTopologyExplicit()
{
  typedef vtkm::cont::internal::DeviceAdapterTraits<
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
  std::cout << "Testing Topology Worklet ( Explicit ) on device adapter: "
            << DeviceAdapterTraits::GetId() << std::endl;

  vtkm::cont::testing::MakeTestDataSet tds;
  vtkm::cont::DataSet *ds = tds.Make3DExplicitDataSet1();

  VTKM_TEST_ASSERT(ds->GetNumberOfCellSets() == 1,
                       "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(ds->GetNumberOfFields() == 6,
                       "Incorrect number of fields");

  vtkm::cont::CellSet *cs = ds->GetCellSet(0);
  vtkm::cont::CellSetExplicit *cse = 
               dynamic_cast<vtkm::cont::CellSetExplicit*>(cs);

  VTKM_TEST_ASSERT(cse, "Expected an explicit cell set");

  //Todo:
  //the scheduling should just be passed a CellSet, and not the
  //derived implementation. The vtkm::cont::CellSet should have
  //a method that return the nodesOfCellsConnectivity / structure
  //for that derived type. ( talk to robert for how dax did this )
  vtkm::worklet::DispatcherMapTopology< ::test::CellValue > dispatcher;
  dispatcher.Invoke(ds->GetField(4).GetData(),
                    ds->GetField(3).GetData(),
                    cse->GetNodeToCellConnectivity(),
                    ds->GetField(5).GetData());


  //cleanup memory
  delete cs;
}

} // anonymous namespace

int UnitTestWorkletMapTopologyExplicit(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestWorkletMapTopologyExplicit);
}
