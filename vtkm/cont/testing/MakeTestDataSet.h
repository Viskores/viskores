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
    vtkm::cont::DataSet Make2DRegularDataSet0();

    // 3D regular datasets.
    vtkm::cont::DataSet Make3DRegularDataSet0();

    // 3D explicit datasets.
    vtkm::cont::DataSet Make3DExplicitDataSet0();
    vtkm::cont::DataSet Make3DExplicitDataSet1();
};


//Make a simple 2D, 2 cell regular dataset.

inline vtkm::cont::DataSet
MakeTestDataSet::Make2DRegularDataSet0()
{
    vtkm::cont::DataSet ds;

    const int nVerts = 6;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1};
    vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f};

    ds.AddField(Field("x", 1, vtkm::cont::Field::ASSOC_POINTS, xVals, nVerts));
    ds.AddField(Field("y", 1, vtkm::cont::Field::ASSOC_POINTS, yVals, nVerts));
    ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("x","y"));

    //set node scalar.
    ds.AddField(Field("nodevar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

    //create scalar.
    vtkm::Float32 cellvar[2] = {100.1f, 200.1f};
    ds.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

    boost::shared_ptr< vtkm::cont::CellSetStructured<2> > cs(
                                new vtkm::cont::CellSetStructured<2>("cells"));
    //Set regular structure
    cs->structure.SetNodeDimension( vtkm::make_Vec(3,2) );
    ds.AddCellSet(cs);

    return ds;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DRegularDataSet0()
{
    vtkm::cont::DataSet ds;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.2f, 60.2f, 70.2f, 80.2f, 90.3f,
                                  100.3f, 110.3f, 120.3f, 130.4f, 140.4f, 150.4f, 160.4f, 170.5f,
                                  180.5f};

    ds.AddField(Field("x", 1, vtkm::cont::Field::ASSOC_POINTS, xVals, nVerts));
    ds.AddField(Field("y", 1, vtkm::cont::Field::ASSOC_POINTS, yVals, nVerts));
    ds.AddField(Field("z", 1, vtkm::cont::Field::ASSOC_POINTS, zVals, nVerts));
    ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("x","y","z"));

    //Set node scalar
    ds.AddField(Field("nodevar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1f, 100.2f, 100.3f, 100.4f};
    ds.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 4));

    static const vtkm::IdComponent dim = 3;
    boost::shared_ptr< vtkm::cont::CellSetStructured<dim> > cs(
                                new vtkm::cont::CellSetStructured<dim>("cells"));
    ds.AddCellSet(cs);

    //Set regular structure
    cs->structure.SetNodeDimension( vtkm::make_Vec(3,2,3) );

    return ds;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet0()
{
  vtkm::cont::DataSet ds;

  const int nVerts = 5;
  vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
  vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
  vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};

  ds.AddField(Field("x", 1, vtkm::cont::Field::ASSOC_POINTS, xVals, nVerts));
  ds.AddField(Field("y", 1, vtkm::cont::Field::ASSOC_POINTS, yVals, nVerts));
  ds.AddField(Field("z", 1, vtkm::cont::Field::ASSOC_POINTS, zVals, nVerts));
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("x","y","z"));

  //Set node scalar
  ds.AddField(Field("nodevar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};
  ds.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

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

  boost::shared_ptr< vtkm::cont::CellSetExplicit<> > cs(
                                new vtkm::cont::CellSetExplicit<>("cells", 2));
  vtkm::cont::ExplicitConnectivity<> &ec = cs->nodesOfCellsConnectivity;
  ec.FillViaCopy(shapes, numindices, conn);

  //todo this need to be a reference/shared_ptr style class
  ds.AddCellSet(cs);

  return ds;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet1()
{
  vtkm::cont::DataSet ds;

  const int nVerts = 5;
  vtkm::Float32 xVals[nVerts] = {0, 1, 1, 2, 2};
  vtkm::Float32 yVals[nVerts] = {0, 0, 1, 1, 2};
  vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0};
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};

  ds.AddField(Field("x", 1, vtkm::cont::Field::ASSOC_POINTS, xVals, nVerts));
  ds.AddField(Field("y", 1, vtkm::cont::Field::ASSOC_POINTS, yVals, nVerts));
  ds.AddField(Field("z", 1, vtkm::cont::Field::ASSOC_POINTS, zVals, nVerts));
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("x","y","z"));

  //Set node scalar
  ds.AddField(Field("nodevar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};
  ds.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

  boost::shared_ptr< vtkm::cont::CellSetExplicit<> > cs(
                                new vtkm::cont::CellSetExplicit<>("cells", 2));
  vtkm::cont::ExplicitConnectivity<> &ec = cs->nodesOfCellsConnectivity;

  ec.PrepareToAddCells(2, 7);
  ec.AddCell(vtkm::VTKM_TRIANGLE, 3, make_Vec<vtkm::Id>(0,1,2));
  ec.AddCell(vtkm::VTKM_QUAD, 4, make_Vec<vtkm::Id>(2,1,3,4));
  ec.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  ds.AddCellSet(cs);

  return ds;
}

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
