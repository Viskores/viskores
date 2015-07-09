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

#include <iostream>
#include <algorithm>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/ExternalFaces.h>


vtkm::Id RunExternalFaces(vtkm::cont::DataSet &ds)
{

  boost::shared_ptr<vtkm::cont::CellSet> scs = ds.GetCellSet(0);
  vtkm::cont::CellSetExplicit<> *cs =
      dynamic_cast<vtkm::cont::CellSetExplicit<> *>(scs.get());

  vtkm::cont::ArrayHandle<vtkm::Id> shapes = cs->GetNodeToCellConnectivity().GetShapesArray();
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices = cs->GetNodeToCellConnectivity().GetNumIndicesArray();
  vtkm::cont::ArrayHandle<vtkm::Id> conn = cs->GetNodeToCellConnectivity().GetConnectivityArray();

  vtkm::Id numExtFaces;

  //Run the External Faces worklet
  vtkm::worklet::ExternalFaces<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().run(
        shapes,
        numIndices,
        conn,
        numExtFaces);

  return numExtFaces;
}

void TestExternalFaces()
{
  //--------------Construct a VTK-m Test Dataset----------------

  vtkm::cont::DataSet ds;

  const int nVerts = 8; //A cube that is tetrahedralized
  vtkm::Float32 xVals[nVerts] = {0, 1, 1, 0, 0, 1, 1, 0};
  vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 0, 0, 1, 1};
  vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 1, 1, 1, 1};
  ds.AddField(vtkm::cont::Field("x", 1, vtkm::cont::Field::ASSOC_POINTS, xVals, nVerts));
  ds.AddField(vtkm::cont::Field("y", 1, vtkm::cont::Field::ASSOC_POINTS, yVals, nVerts));
  ds.AddField(vtkm::cont::Field("z", 1, vtkm::cont::Field::ASSOC_POINTS, zVals, nVerts));
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("x","y","z"));

  //Construct the VTK-m shapes and numIndices connectivity arrays
  const int nCells = 6;  //The tetrahedrons of the cube
  int cellVerts[nCells][4] = {{4,7,6,3}, {4,6,3,2}, {4,0,3,2},
                             {4,6,5,2}, {4,5,0,2}, {1,0,5,2}};
  boost::shared_ptr< vtkm::cont::CellSetExplicit<> > cs(
                              new vtkm::cont::CellSetExplicit<>("cells", nCells));
  vtkm::cont::ArrayHandle<vtkm::Id> shapes;
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> conn;
  shapes.Allocate(static_cast<vtkm::Id>(nCells));
  numIndices.Allocate(static_cast<vtkm::Id>(nCells));
  conn.Allocate(static_cast<vtkm::Id>(4 * nCells));

  int index = 0;
  for(int j = 0; j < nCells; j++)
  {
      shapes.GetPortalControl().Set(j, static_cast<vtkm::Id>(vtkm::VTKM_TETRA));
      numIndices.GetPortalControl().Set(j, 4);
      for(int k = 0; k < 4; k++)
        conn.GetPortalControl().Set(index++, static_cast<vtkm::Id>(cellVerts[j][k]));
  }

  vtkm::cont::ExplicitConnectivity<> &ec = cs->nodesOfCellsConnectivity;
  ec.Fill(shapes, numIndices, conn);

  //Add the VTK-m cell set
  ds.AddCellSet(cs);
  ds.PrintSummary(std::cout);

  //Run the External Faces worklet
  vtkm::Id numExtFaces_out = RunExternalFaces(ds);

  //Validate the number of external faces (output) returned by the worklet
  const vtkm::Id numExtFaces_actual = 12;
  VTKM_TEST_ASSERT(numExtFaces_out == numExtFaces_actual, "Number of External Faces mismatch");

} // TestExternalFaces


int UnitTestExternalFaces(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestExternalFaces);
}
