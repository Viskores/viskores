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
    vtkm::cont::DataSet Make3DExplicitDataSetCowNose(double *pBounds = NULL);
};


//Make a simple 2D, 2 cell regular dataset.

inline vtkm::cont::DataSet
MakeTestDataSet::Make2DRegularDataSet0()
{
    vtkm::cont::DataSet dataSet;

    const int nVerts = 6;
    vtkm::cont::ArrayHandleUniformPointCoordinates
        coordinates(vtkm::Id3(3, 2, 1));
    vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f};

    dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

    //set point scalar.
    dataSet.AddField(Field("pointvar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

    //create scalar.
    vtkm::Float32 cellvar[2] = {100.1f, 200.1f};
    dataSet.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

    vtkm::cont::CellSetStructured<2> cellSet("cells");
    //Set regular structure
    cellSet.SetPointDimensions( vtkm::make_Vec(3,2) );
    dataSet.AddCellSet(cellSet);

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
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

    //Set point scalar
    dataSet.AddField(Field("pointvar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1f, 100.2f, 100.3f, 100.4f};
    dataSet.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 4));

    static const vtkm::IdComponent dim = 3;
    vtkm::cont::CellSetStructured<dim> cellSet("cells");
    cellSet.SetPointDimensions( vtkm::make_Vec(3,2,3) );
    dataSet.AddCellSet(cellSet);

    return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet0()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 5;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),
    CoordType(1, 0, 0),
    CoordType(1, 1, 0),
    CoordType(2, 1, 0),
    CoordType(2, 2, 0)
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", 1, coordinates, nVerts));

  //Set point scalar
  dataSet.AddField(Field("pointvar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};
  dataSet.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

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

  vtkm::cont::CellSetExplicit<> cellSet("cells", 2);
  cellSet.FillViaCopy(shapes, numindices, conn);

  dataSet.AddCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSet1()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 5;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),
    CoordType(1, 0, 0),
    CoordType(1, 1, 0),
    CoordType(2, 1, 0),
    CoordType(2, 2, 0)
  };
  vtkm::Float32 vars[nVerts] = {10.1f, 20.1f, 30.2f, 40.2f, 50.3f};

  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", 1, coordinates, nVerts));

  //Set point scalar
  dataSet.AddField(Field("pointvar", 1, vtkm::cont::Field::ASSOC_POINTS, vars, nVerts));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = {100.1f, 100.2f};
  dataSet.AddField(Field("cellvar", 1, vtkm::cont::Field::ASSOC_CELL_SET, "cells", cellvar, 2));

  vtkm::cont::CellSetExplicit<> cellSet("cells", 2);

  cellSet.PrepareToAddCells(2, 7);
  cellSet.AddCell(vtkm::VTKM_TRIANGLE, 3, make_Vec<vtkm::Id>(0,1,2));
  cellSet.AddCell(vtkm::VTKM_QUAD, 4, make_Vec<vtkm::Id>(2,1,3,4));
  cellSet.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet
MakeTestDataSet::Make3DExplicitDataSetCowNose(double *pBounds)
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
  double _bounds[6] = {-0.000169, 0.048088, 0.001378, 0.250062, 0.053925, 0.200755};

  // create DataSet
  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", 1, coordinates, nVerts));

  vtkm::cont::CellSetExplicit<> cellSet("cells", 2);

  cellSet.PrepareToAddCells(nPointIds/3, nPointIds);
  for (vtkm::Id i=0; i<nPointIds/3; i++)
  {
    cellSet.AddCell(vtkm::VTKM_TRIANGLE, 3, make_Vec<vtkm::Id>(pointId[i*3], pointId[i*3+1], pointId[i*3+2]));
  }
  cellSet.CompleteAddingCells();

  //todo this need to be a reference/shared_ptr style class
  dataSet.AddCellSet(cellSet);

  // copy bounds
  if (pBounds != NULL)
  {
    for (vtkm::IdComponent i=0; i<6; i++)
    {
      pBounds[i] = _bounds[i];
    }
  }

  return dataSet;
}
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
