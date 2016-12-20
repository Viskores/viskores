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

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

namespace vtkm {
namespace cont {
namespace testing {

class MakeTestDataSet
{
public:
    // 1D uniform datasets.
    vtkm::cont::DataSet Make1DUniformDataSet0();
    // 1D explicit datasets.
    vtkm::cont::DataSet Make1DExplicitDataSet0();
    
    // 2D uniform datasets.
    vtkm::cont::DataSet Make2DUniformDataSet0();

    // 3D uniform datasets.
    vtkm::cont::DataSet Make3DUniformDataSet0();
    vtkm::cont::DataSet Make3DRegularDataSet0();
    vtkm::cont::DataSet Make3DRegularDataSet1();

    //2D rectilinear
    vtkm::cont::DataSet Make2DRectilinearDataSet0();

    //3D rectilinear
    vtkm::cont::DataSet Make3DRectilinearDataSet0();


    // 3D explicit datasets.
    vtkm::cont::DataSet Make3DExplicitDataSet0();
    vtkm::cont::DataSet Make3DExplicitDataSet1();
    vtkm::cont::DataSet Make3DExplicitDataSet2();
    vtkm::cont::DataSet Make3DExplicitDataSet3();
    vtkm::cont::DataSet Make3DExplicitDataSet4();
    vtkm::cont::DataSet Make3DExplicitDataSet5();
    vtkm::cont::DataSet Make3DExplicitDataSetCowNose();
};

//Make a simple 1D dataset.
inline vtkm::cont::DataSet
MakeTestDataSet::Make1DUniformDataSet0()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  const vtkm::Id nVerts = 6;
  vtkm::cont::DataSet dataSet = dsb.Create(nVerts);

  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::Float32 var[nVerts] = {-1.0f, .5f, -.2f, 1.7f, -.1f, .8f};
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);
  
  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make1DExplicitDataSet0()
{
  const int nVerts = 5;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(nVerts);
  coords[0] = CoordType(0.0f,0.f,0.f);
  coords[1] = CoordType(1.0f,0.f,0.f);
  coords[2] = CoordType(1.1f,0.f,0.f);
  coords[3] = CoordType(1.2f,0.f,0.f);
  coords[4] = CoordType(4.0f,0.f,0.f);

  std::vector<vtkm::Id> conn;
  for (int i = 0; i < nVerts; i++)
      conn.push_back(i);

  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  dataSet = dsb.Create(coords, vtkm::CellShapeTagLine(), conn, "coordinates", "cells");

  vtkm::cont::DataSetFieldAdd dsf;  
  vtkm::Float32 var[nVerts] = {-1.0f, .5f, -.2f, 1.7f, .8f};
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  return dataSet;
}

//Make a simple 2D, 2 cell uniform dataset.

inline vtkm::cont::DataSet
MakeTestDataSet::Make2DUniformDataSet0()
{
    vtkm::cont::DataSetBuilderUniform dsb;
    vtkm::Id2 dimensions(3,2);
    vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

    vtkm::cont::DataSetFieldAdd dsf;
    const vtkm::Id nVerts = 6;
    vtkm::Float32 var[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f};

    dsf.AddPointField(dataSet, "pointvar", var, nVerts);

    vtkm::Float32 cellvar[2] = {100.1f, 200.1f};
    dsf.AddCellField(dataSet, "cellvar", cellvar, 2, "cells");

    return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DUniformDataSet0()
{
    vtkm::cont::DataSetBuilderUniform dsb;
    vtkm::Id3 dimensions(3,2,3);
    vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

    vtkm::cont::DataSetFieldAdd dsf;
    const int nVerts = 18;
    vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.2f,
                                  60.2f, 70.2f, 80.2f, 90.3f, 100.3f,
                                  110.3f, 120.3f, 130.4f, 140.4f,
                                  150.4f, 160.4f, 170.5f, 180.5f};

    //Set point and cell scalar
    dsf.AddPointField(dataSet, "pointvar", vars, nVerts);

    vtkm::Float32 cellvar[4] = {100.1f, 100.2f, 100.3f, 100.4f};
    dsf.AddCellField(dataSet, "cellvar", cellvar, 4, "cells");

    return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make2DRectilinearDataSet0()
{
    vtkm::cont::DataSetBuilderRectilinear dsb;
    std::vector<vtkm::Float32> X(3), Y(2);

    X[0] = 0.0f;
    X[1] = 1.0f;
    X[2] = 2.0f;
    Y[0] = 0.0f;
    Y[1] = 1.0f;

    vtkm::cont::DataSet dataSet = dsb.Create(X, Y);

    vtkm::cont::DataSetFieldAdd dsf;
    const vtkm::Id nVerts = 6;
    vtkm::Float32 var[nVerts];
    for (int i = 0; i < nVerts; i++)
        var[i] = (vtkm::Float32)i;
    dsf.AddPointField(dataSet, "pointvar", var, nVerts);

    const vtkm::Id nCells = 2;
    vtkm::Float32 cellvar[nCells];
    for (int i = 0; i < nCells; i++)
        cellvar[i] = (vtkm::Float32)i;
    dsf.AddCellField(dataSet, "cellvar", cellvar, nCells, "cells");

    return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DRegularDataSet0()
{
    vtkm::cont::DataSet dataSet;

    const int nVerts = 18;
    vtkm::cont::ArrayHandleUniformPointCoordinates
        coordinates(vtkm::Id3(3, 2, 3));
    vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.2f, 60.2f, 70.2f, 80.2f, 90.3f,
                                  100.3f, 110.3f, 120.3f, 130.4f, 140.4f, 150.4f, 160.4f, 170.5f,
                                  180.5f};

    dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", coordinates));

    //Set point scalar
    dataSet.AddField(Field("pointvar", vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1f, 100.2f, 100.3f, 100.4f};
    dataSet.AddField(Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 4));

    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> cellSet("cells");
    cellSet.SetPointDimensions( vtkm::make_Vec(3,2,3) );
    dataSet.AddCellSet(cellSet);

    return dataSet;
}


inline vtkm::cont::DataSet
MakeTestDataSet::Make3DRegularDataSet1()
{
    vtkm::cont::DataSet dataSet;

    const int nVerts = 8;
    vtkm::cont::ArrayHandleUniformPointCoordinates
        coordinates(vtkm::Id3(2, 2, 2));
    vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.2f, 60.2f, 70.2f, 80.2f};

    dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", coordinates));

    //Set point scalar
    dataSet.AddField(Field("pointvar", vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

    //Set cell scalar
    vtkm::Float32 cellvar[1] = {100.1f};
    dataSet.AddField(Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 1));

    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> cellSet("cells");
    cellSet.SetPointDimensions( vtkm::make_Vec(2,2,2) );
    dataSet.AddCellSet(cellSet);

    return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DRectilinearDataSet0()
{
    vtkm::cont::DataSetBuilderRectilinear dsb;
    std::vector<vtkm::Float32> X(3), Y(2), Z(3);

    X[0] = 0.0f;
    X[1] = 1.0f;
    X[2] = 2.0f;
    Y[0] = 0.0f;
    Y[1] = 1.0f;
    Z[0] = 0.0f;
    Z[1] = 1.0f;
    Z[2] = 2.0f;

    vtkm::cont::DataSet dataSet = dsb.Create(X, Y, Z);

    vtkm::cont::DataSetFieldAdd dsf;
    const vtkm::Id nVerts = 18;
    vtkm::Float32 var[nVerts];
    for (int i = 0; i < nVerts; i++)
        var[i] = (vtkm::Float32)i;
    dsf.AddPointField(dataSet, "pointvar", var, nVerts);

    const vtkm::Id nCells = 4;
    vtkm::Float32 cellvar[nCells];
    for (int i = 0; i < nCells; i++)
        cellvar[i] = (vtkm::Float32)i;
    dsf.AddCellField(dataSet, "cellvar", cellvar, nCells, "cells");

    return dataSet;
 }

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet0()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 5;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(nVerts);
  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(1, 1, 0);
  coords[3] = CoordType(2, 1, 0);
  coords[4] = CoordType(2, 2, 0);

  //Connectivity
  std::vector<vtkm::UInt8> shapes;
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);

  std::vector<vtkm::IdComponent> numindices;
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

  //Create the dataset.
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates", "cells");

  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, 2, "cells");

  return dataSet;
}
    /*
inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet1()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetIterativeBuilderExplicit dsb;
  vtkm::Id id0, id1, id2, id3, id4;

  dsb.Begin("coords", "cells");

  id0 = dsb.AddPoint(0,0,0);
  id1 = dsb.AddPoint(1,0,0);
  id2 = dsb.AddPoint(1,1,0);
  id3 = dsb.AddPoint(2,1,0);
  id4 = dsb.AddPoint(2,2,0);

  vtkm::Id ids0[3] = {id0, id1, id2};
  dsb.AddCell(vtkm::CELL_SHAPE_TRIANGLE, ids0, 3);

  vtkm::Id ids1[4] = {id2, id1, id3, id4};
  dsb.AddCell(vtkm::CELL_SHAPE_QUAD, ids1, 4);
  dataSet = dsb.Create();

  vtkm::Float32 vars[5] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", vars, 5);
  dsf.AddCellField(dataSet, "cellvar", cellvar, 2, "cells");

  return dataSet;
}
    */


inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet1()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 5;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(1, 1, 0);
  coords[3] = CoordType(2, 1, 0);
  coords[4] = CoordType(2, 2, 0);
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),
    CoordType(1, 0, 0),
    CoordType(1, 1, 0),
    CoordType(2, 1, 0),
    CoordType(2, 2, 0)
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));
  vtkm::cont::CellSetExplicit<> cellSet(nVerts, "cells");
  cellSet.PrepareToAddCells(2, 7);
  cellSet.AddCell(vtkm::CELL_SHAPE_TRIANGLE, 3, make_Vec<vtkm::Id>(0,1,2));
  cellSet.AddCell(vtkm::CELL_SHAPE_QUAD, 4, make_Vec<vtkm::Id>(2,1,3,4));
  cellSet.CompleteAddingCells();
  dataSet.AddCellSet(cellSet);

  //Set point scalar
  dataSet.AddField(Field("pointvar", vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};
  dataSet.AddField(Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet2()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 8;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), // 0
    CoordType(1, 0, 0), // 1
    CoordType(1, 0, 1), // 2
    CoordType(0, 0, 1), // 3
    CoordType(0, 1, 0), // 4
    CoordType(1, 1, 0), // 5
    CoordType(1, 1, 1), // 6
    CoordType(0, 1, 1)  // 7
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f, 70.2f, 80.3f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));

  //Set point scalar
  dataSet.AddField(Field("pointvar", vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f};
  dataSet.AddField(Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 1));

  vtkm::cont::CellSetExplicit<> cellSet(nVerts,"cells");
  vtkm::Vec<vtkm::Id, 8> ids;
  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 2;
  ids[3] = 3;
  ids[4] = 4;
  ids[5] = 5;
  ids[6] = 6;
  ids[7] = 7;

  cellSet.PrepareToAddCells(1, 8);
  cellSet.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);
  cellSet.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet4()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 12;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), //0
    CoordType(1, 0, 0), //1
    CoordType(1, 0, 1), //2
    CoordType(0, 0, 1), //3
    CoordType(0, 1, 0), //4
    CoordType(1, 1, 0), //5
    CoordType(1, 1, 1), //6
    CoordType(0, 1, 1), //7
    CoordType(2, 0, 0), //8
    CoordType(2, 0, 1), //9
    CoordType(2, 1, 1), //10
    CoordType(2, 1, 0)  //11
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f, 70.2f, 80.3f, 90.f, 10.f, 11.f, 12.f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));

  //Set point scalar
  dataSet.AddField(Field("pointvar", vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f, 110.f};
  dataSet.AddField(Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

  vtkm::cont::CellSetExplicit<> cellSet(nVerts,"cells");
  vtkm::Vec<vtkm::Id, 8> ids;
  ids[0] = 0;
  ids[1] = 4;
  ids[2] = 5;
  ids[3] = 1;
  ids[4] = 3;
  ids[5] = 7;
  ids[6] = 6;
  ids[7] = 2;

  cellSet.PrepareToAddCells(2, 16);
  cellSet.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);
  ids[0] = 1;
  ids[1] = 5;
  ids[2] = 11;
  ids[3] = 8;
  ids[4] = 2;
  ids[5] = 6;
  ids[6] = 10;
  ids[7] = 9;
  cellSet.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);
  cellSet.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet3()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 4;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),
    CoordType(1, 0, 0),
    CoordType(1, 0, 1),
    CoordType(0, 1, 0)
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 10.1f, 10.2f, 30.2f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));

  //Set point scalar
  dataSet.AddField(Field("pointvar", vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f};
  dataSet.AddField(Field("cellvar", vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 1));

  vtkm::cont::CellSetExplicit<> cellSet(nVerts,"cells");
  vtkm::Vec<vtkm::Id, 4> ids;
  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 2;
  ids[3] = 3;

  cellSet.PrepareToAddCells(1, 4);
  cellSet.AddCell(vtkm::CELL_SHAPE_TETRA, 4, ids);
  cellSet.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet5()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 11;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), //0
    CoordType(1, 0, 0), //1
    CoordType(1, 0, 1), //2
    CoordType(0, 0, 1), //3
    CoordType(0, 1, 0), //4
    CoordType(1, 1, 0), //5
    CoordType(1, 1, 1), //6
    CoordType(0, 1, 1), //7
    CoordType(2, 0, 0), //8
    CoordType(0, 2, 0), //9
    CoordType(1, 2, 0)  //10
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f, 70.2f, 80.3f, 90.f, 10.f, 11.f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));

  //Set point scalar
  dataSet.AddField(Field("pointvar",
                         vtkm::cont::Field::ASSOC_POINTS,
                         vars,
                         nVerts));

  //Set cell scalar
  const int nCells = 4;
  vtkm::Float32 cellvar[nCells] = {100.1f, 110.f, 120.2f, 130.5f};
  dataSet.AddField(Field("cellvar",
                         vtkm::cont::Field::ASSOC_CELL_SET,
                         "cells",
                         cellvar,
                         nCells));

  vtkm::cont::CellSetExplicit<> cellSet(nVerts,"cells");
  vtkm::Vec<vtkm::Id, 8> ids;

  cellSet.PrepareToAddCells(nCells, 23);

  ids[0] = 0;
  ids[1] = 4;
  ids[2] = 5;
  ids[3] = 1;
  ids[4] = 3;
  ids[5] = 7;
  ids[6] = 6;
  ids[7] = 2;
  cellSet.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);

  ids[0] = 1;
  ids[1] = 2;
  ids[2] = 6;
  ids[3] = 5;
  ids[4] = 8;
  cellSet.AddCell(vtkm::CELL_SHAPE_PYRAMID, 5, ids);

  ids[0] = 5;
  ids[1] = 10;
  ids[2] = 8;
  ids[3] = 6;
  cellSet.AddCell(vtkm::CELL_SHAPE_TETRA, 4, ids);

  ids[0] = 4;
  ids[1] = 9;
  ids[2] = 7;
  ids[3] = 5;
  ids[4] = 10;
  ids[5] = 6;
  cellSet.AddCell(vtkm::CELL_SHAPE_WEDGE, 6, ids);

  cellSet.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSetCowNose()
{
  // prepare data array
  const int nVerts = 17;
  typedef vtkm::Vec<vtkm::Float64,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0.0480879,0.151874,0.107334),
    CoordType(0.0293568,0.245532,0.125337),
    CoordType(0.0224398,0.246495,0.1351),
    CoordType(0.0180085,0.20436,0.145316),
    CoordType(0.0307091,0.152142,0.0539249),
    CoordType(0.0270341,0.242992,0.107567),
    CoordType(0.000684071,0.00272505,0.175648),
    CoordType(0.00946217,0.077227,0.187097),
    CoordType(-0.000168991,0.0692243,0.200755),
    CoordType(-0.000129414,0.00247137,0.176561),
    CoordType(0.0174172,0.137124,0.124553),
    CoordType(0.00325994,0.0797155,0.184912),
    CoordType(0.00191765,0.00589327,0.16608),
    CoordType(0.0174716,0.0501928,0.0930275),
    CoordType(0.0242103,0.250062,0.126256),
    CoordType(0.0108188,0.152774,0.167914),
    CoordType(5.41687e-05,0.00137834,0.175119)
  };
  const int nPointIds = 57;
  vtkm::Id pointId[nPointIds] = {
    0, 1, 3,
    2, 3, 1,
    4, 5, 0,
    1, 0, 5,
    7, 8, 6,
    9, 6, 8,
    0, 10, 7,
    11, 7, 10,
    0, 6, 13,
    12, 13, 6,
    1, 5, 14,
    1, 14, 2,
    0, 3, 15,
    0, 13, 4,
    6, 16, 12,
    6, 9, 16,
    7, 11, 8,
    0, 15, 10,
    7, 6, 0
  };

  // create DataSet
  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  connectivity.Allocate(nPointIds);

  for(vtkm::Id i=0; i < nPointIds; ++i)
  {
    connectivity.GetPortalControl().Set(i, pointId[i]);
  }
  vtkm::cont::CellSetSingleType< > cellSet(vtkm::CellShapeTagTriangle(),
                                           nVerts,
                                           "cells");
  cellSet.Fill(connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
