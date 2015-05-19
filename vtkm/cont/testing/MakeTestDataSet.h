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

#ifndef vtk_m_cont_testing_MakeTestDataSet_h
#define vtk_m_cont_testing_MakeTestDataSet_h

#include <vtkm/cont/DataSet.h>

namespace vtkm {
namespace cont {
namespace testing {

class MakeTestDataSet
{
public:
    // 2D regular datasets.
    vtkm::cont::DataSet * Make2DRegularDataSet0();

    // 3D regular datasets.
    vtkm::cont::DataSet * Make3DRegularDataSet0();

    // 3D explicit datasets.
    vtkm::cont::DataSet * Make3DExplicitDataSet0();
    vtkm::cont::DataSet * Make3DExplicitDataSet1();
};


//Make a simple 2D, 2 cell regular dataset.

inline vtkm::cont::DataSet *
MakeTestDataSet::Make2DRegularDataSet0()
{
    vtkm::cont::DataSet *ds = new vtkm::cont::DataSet;
    ds->x_idx = 0;
    ds->y_idx = 1;
    ds->z_idx = -1;

    const int nVerts = 6;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.1, 60.1};
    ds->AddFieldViaCopy(xVals, nVerts);
    ds->AddFieldViaCopy(yVals, nVerts);
    
    //set node scalar.
    ds->AddFieldViaCopy(vars, nVerts);

    vtkm::Float32 cellvar[2] = {100.1, 200.1};
    ds->AddFieldViaCopy(cellvar, 2);

    vtkm::cont::CellSetStructured<2> *cs = new vtkm::cont::CellSetStructured<2>("cells");
    //Set regular structure
    cs->structure.SetNodeDimension(3,2);
    ds->AddCellSet(cs);

    return ds;
}

inline vtkm::cont::DataSet *
MakeTestDataSet::Make3DRegularDataSet0()
{
    vtkm::cont::DataSet *ds = new vtkm::cont::DataSet;

    ds->x_idx = 0;
    ds->y_idx = 1;
    ds->z_idx = 2;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2, 60.2, 70.2, 80.2, 90.3,
                                  100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5,
                                  180.5};

    ds->AddFieldViaCopy(xVals, nVerts);
    ds->AddFieldViaCopy(yVals, nVerts);
    ds->AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds->AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    ds->AddFieldViaCopy(cellvar, 4);
    
    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> *cs = new vtkm::cont::CellSetStructured<dim>("cells");
    ds->AddCellSet(cs);

    //Set regular structure
    cs->structure.SetNodeDimension(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    ds->AddFieldViaCopy(cellVals, 4);

    return ds;
}

inline vtkm::cont::DataSet *
MakeTestDataSet::Make3DExplicitDataSet0()
{
  vtkm::cont::DataSet *ds = new vtkm::cont::DataSet;

  ds->x_idx = 0;
  ds->y_idx = 1;
  ds->z_idx = 2;

  const int nVerts = 5;
  vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
  vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
  vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
  vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.2, 40.2, 50.3};


  ds->AddFieldViaCopy(xVals, nVerts);
  ds->AddFieldViaCopy(yVals, nVerts);
  ds->AddFieldViaCopy(zVals, nVerts);

  //Set node scalar
  ds->AddFieldViaCopy(vars, nVerts);

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1, 100.2};
  ds->AddFieldViaCopy(cellvar, 2);

  //Add connectivity
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

  vtkm::cont::CellSetExplicit *cs = new vtkm::cont::CellSetExplicit("cells",2);
  vtkm::cont::ExplicitConnectivity &ec = cs->nodesOfCellsConnectivity;

  ec.FillViaCopy(shapes, numindices, conn);

  //todo this need to be a reference/shared_ptr style class
  ds->AddCellSet(cs);

  //Run a worklet to populate a cell centered field.
  //Here, we're filling it with test values.
  vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
  ds->AddFieldViaCopy(outcellVals, 2);

  return ds;
}

inline vtkm::cont::DataSet *
MakeTestDataSet::Make3DExplicitDataSet1()
{
  vtkm::cont::DataSet *ds = new vtkm::cont::DataSet;

  ds->x_idx = 0;
  ds->y_idx = 1;
  ds->z_idx = 2;

  const int nVerts = 5;
  vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
  vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
  vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
  vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.2, 40.2, 50.3};


  ds->AddFieldViaCopy(xVals, nVerts);
  ds->AddFieldViaCopy(yVals, nVerts);
  ds->AddFieldViaCopy(zVals, nVerts);

  //Set node scalar
  ds->AddFieldViaCopy(vars, nVerts);

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1, 100.2};
  ds->AddFieldViaCopy(cellvar, 2);

  vtkm::cont::CellSetExplicit *cs = new vtkm::cont::CellSetExplicit("cells",2);
  vtkm::cont::ExplicitConnectivity &ec = cs->nodesOfCellsConnectivity;

  ec.PrepareToAddCells(2, 7);
  ec.AddCell(vtkm::VTKM_TRIANGLE, 3, make_Vec<vtkm::Id>(0,1,2));
  ec.AddCell(vtkm::VTKM_QUAD, 4, make_Vec<vtkm::Id>(2,1,3,4));
  ec.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  ds->AddCellSet(cs);

  //Run a worklet to populate a cell centered field.
  //Here, we're filling it with test values.
  vtkm::Float32 outcellVals[2] = {-1.4, -1.7};
  ds->AddFieldViaCopy(outcellVals, 2);

  return ds;
}

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
