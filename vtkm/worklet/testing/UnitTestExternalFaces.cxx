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
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/ExternalFaces.h>

namespace {

vtkm::cont::DataSet RunExternalFaces(vtkm::cont::DataSet &ds)
{

  vtkm::cont::CellSetExplicit<> &cellset =
      ds.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

  vtkm::cont::ArrayHandle<vtkm::Id> shapes = cellset.GetShapesArray(
    vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices = cellset.GetNumIndicesArray(
    vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());
  vtkm::cont::ArrayHandle<vtkm::Id> conn = cellset.GetConnectivityArray(
    vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell());

  vtkm::cont::ArrayHandle<vtkm::Id> output_shapes;
  vtkm::cont::ArrayHandle<vtkm::Id> output_numIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> output_conn;

  //Run the External Faces worklet
  vtkm::worklet::ExternalFaces<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().run(
        shapes,
        numIndices,
        conn,
        output_shapes,
        output_numIndices,
        output_conn);

  vtkm::cont::DataSet new_ds;
  for(vtkm::IdComponent i=0; i < ds.GetNumberOfCoordinateSystems(); ++i)
  {
    new_ds.AddCoordinateSystem(ds.GetCoordinateSystem(i));
  }


  vtkm::cont::CellSetExplicit<> new_cs(cellset.GetNumberOfPoints(),"cells", 2);
  new_cs.Fill(output_shapes, output_numIndices, output_conn);
  new_ds.AddCellSet(new_cs);

  return new_ds;
}

void TestExternalFaces()
{
  //--------------Construct a VTK-m Test Dataset----------------

  vtkm::cont::DataSet ds;

  const int nVerts = 8; //A cube that is tetrahedralized
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),
    CoordType(1, 0, 0),
    CoordType(1, 1, 0),
    CoordType(0, 1, 0),
    CoordType(0, 0, 1),
    CoordType(1, 0, 1),
    CoordType(1, 1, 1),
    CoordType(0, 1, 1)
    };

  ds.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem("coordinates", 1, coordinates, nVerts));

  //Construct the VTK-m shapes and numIndices connectivity arrays
  const int nCells = 6;  //The tetrahedrons of the cube
  int cellVerts[nCells][4] = {{4,7,6,3}, {4,6,3,2}, {4,0,3,2},
                             {4,6,5,2}, {4,5,0,2}, {1,0,5,2}};
  vtkm::cont::CellSetExplicit<> cs(nVerts, "cells", nCells);

  vtkm::cont::ArrayHandle<vtkm::Id> shapes;
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> conn;
  shapes.Allocate(static_cast<vtkm::Id>(nCells));
  numIndices.Allocate(static_cast<vtkm::Id>(nCells));
  conn.Allocate(static_cast<vtkm::Id>(4 * nCells));

  int index = 0;
  for(int j = 0; j < nCells; j++)
  {
    shapes.GetPortalControl().Set(j, static_cast<vtkm::Id>(vtkm::CELL_SHAPE_TETRA));
    numIndices.GetPortalControl().Set(j, 4);
    for(int k = 0; k < 4; k++)
      conn.GetPortalControl().Set(index++, static_cast<vtkm::Id>(cellVerts[j][k]));
  }

  cs.Fill(shapes, numIndices, conn);

  //Add the VTK-m cell set
  ds.AddCellSet(cs);

  //Run the External Faces worklet
  vtkm::cont::DataSet new_ds = RunExternalFaces(ds);
  vtkm::cont::CellSetExplicit<> &new_cs =
      new_ds.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

  vtkm::Id numExtFaces_out = new_cs.GetNumberOfCells();

  //Validate the number of external faces (output) returned by the worklet
  const vtkm::Id numExtFaces_actual = 12;
  VTKM_TEST_ASSERT(numExtFaces_out == numExtFaces_actual, "Number of External Faces mismatch");

} // TestExternalFaces

}

int UnitTestExternalFaces(int, char *[])
{
      return vtkm::cont::testing::Testing::Run(TestExternalFaces);
}
