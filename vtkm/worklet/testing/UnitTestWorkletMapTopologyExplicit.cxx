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

  vtkm::cont::DataSet ds;

  ds.x_idx = 0;
  ds.y_idx = 1;
  ds.z_idx = 2;

  const int nVerts = 5;
  vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
  vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
  vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
  vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.2, 40.2, 50.3};


  ds.AddFieldViaCopy(xVals, nVerts);
  ds.AddFieldViaCopy(yVals, nVerts);
  ds.AddFieldViaCopy(zVals, nVerts);

  //Set node scalar
  ds.AddFieldViaCopy(vars, nVerts);

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1, 100.2};
  ds.AddFieldViaCopy(cellvar, 2);

  //Add connectivity
  vtkm::cont::ArrayHandle<vtkm::Id> tmp2;
  std::vector<vtkm::Id> shapes;
  shapes.push_back(vtkm::VTKM_TRIANGLE);
  shapes.push_back(vtkm::VTKM_QUAD);

  std::vector<vtkm::Id> numindices;
  numindices.push_back(3);
  numindices.push_back(4);

  std::vector<vtkm::Id> conn;
  // First Cell: Triangle
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  // Second Cell: Quad
  conn.push_back(2);
  conn.push_back(1);
  conn.push_back(3);
  conn.push_back(4);

  std::vector<vtkm::Id> map_cell_to_index;
  map_cell_to_index.push_back(0);
  map_cell_to_index.push_back(3);

  vtkm::cont::ExplicitConnectivity ec;
  ec.Shapes = vtkm::cont::make_ArrayHandle(shapes);
  ec.NumIndices = vtkm::cont::make_ArrayHandle(numindices);
  ec.Connectivity = vtkm::cont::make_ArrayHandle(conn);
  ec.MapCellToConnectivityIndex = vtkm::cont::make_ArrayHandle(map_cell_to_index);

  //todo this need to be a reference/shared_ptr style class
  vtkm::cont::CellSetExplicit *cs = new vtkm::cont::CellSetExplicit("cells",2);
  cs->nodesOfCellsConnectivity = ec;
  ds.AddCellSet(cs);

  //Run a worklet to populate a cell centered field.
  //Here, we're filling it with test values.
  vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
  ds.AddFieldViaCopy(outcellVals, 2);


  VTKM_TEST_ASSERT(test_equal(ds.GetNumberOfCellSets(), 1),
                       "Incorrect number of cell sets");

  VTKM_TEST_ASSERT(test_equal(ds.GetNumberOfFields(), 6),
                       "Incorrect number of fields");

  //Todo:
  //the scheduling should just be passed a CellSet, and not the
  //derived implementation. The vtkm::cont::CellSet should have
  //a method that return the nodesOfCellsConnectivity / structure
  //for that derived type. ( talk to robert for how dax did this )
  vtkm::worklet::DispatcherMapTopology< ::test::CellValue > dispatcher;
  dispatcher.Invoke(ds.GetField(4).GetData(),
                    ds.GetField(3).GetData(),
                    cs->nodesOfCellsConnectivity,
                    ds.GetField(5).GetData());


  //cleanup memory
  delete cs;
}

} // anonymous namespace

int UnitTestWorkletMapTopologyExplicit(int, char *[])
{
    return vtkm::cont::testing::Testing::Run(TestWorkletMapTopologyExplicit);
}
