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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace {

void TestDataSet_Explicit()
{
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

  //todo this need to be a reference
  vtkm::cont::CellSetExplicit *cs = new vtkm::cont::CellSetExplicit("cells",2);
  cs->nodesOfCellsConnectivity = ec;

  //Run a worklet to populate a cell centered field.
  //Here, we're filling it with test values.
  vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
  ds.AddFieldViaCopy(outcellVals, 2);
}

}


int UnitTestDataSetExplicit(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Explicit);
}
