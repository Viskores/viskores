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

#ifndef vtk_m_cont_testing_MakeFilterDataSet_h
#define vtk_m_cont_testing_MakeFilterDataSet_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

namespace vtkm {
namespace cont {
namespace testing {

class MakeFilterDataSet
{
public:
    vtkm::cont::DataSet Make2DUniformFilterDataSet();
    vtkm::cont::DataSet Make3DUniformFilterDataSet();

    vtkm::cont::DataSet Make2DExplicitFilterDataSet();
    vtkm::cont::DataSet Make3DExplicitFilterDataSet();
};

//
// Create 2D uniform dataset
//
inline vtkm::cont::DataSet 
MakeFilterDataSet::Make2DUniformFilterDataSet()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(5,5);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  const vtkm::Id nVerts = 25;
  const vtkm::Id nCells = 16;
  vtkm::Float32 pointvar[nVerts] = {
               100.0f, 78.0f, 49.0f, 17.0f,  1.0f,
                94.0f, 71.0f, 47.0f, 33.0f,  6.0f,
                52.0f, 44.0f, 50.0f, 45.0f, 48.0f,
                 8.0f, 12.0f, 46.0f, 91.0f, 43.0f,
                 0.0f,  5.0f, 51.0f, 76.0f, 83.0f};
  vtkm::Float32 cellvar[nCells] = {
                 0.0f,  1.0f,  2.0f,  3.0f,
                 4.0f,  5.0f,  6.0f,  7.0f,
                 8.0f,  9.0f, 10.0f, 11.0f,
                12.0f, 13.0f, 14.0f, 15.0f};

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells, "cells");

  return dataSet;
}

//
// Create 3D uniform dataset
//
inline vtkm::cont::DataSet 
MakeFilterDataSet::Make3DUniformFilterDataSet()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id3 dimensions(5,5,5);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  const vtkm::Id nVerts = 125;
  const vtkm::Id nCells = 64;
  vtkm::Float32 pointvar[nVerts] = {
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f, 99.0f, 90.0f, 85.0f,  0.0f,
                 0.0f, 95.0f, 80.0f, 95.0f,  0.0f,
                 0.0f, 85.0f, 90.0f, 99.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f, 75.0f, 50.0f, 65.0f,  0.0f,
                 0.0f, 55.0f, 15.0f, 45.0f,  0.0f,
                 0.0f, 60.0f, 40.0f, 70.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f, 97.0f, 87.0f, 82.0f,  0.0f,
                 0.0f, 92.0f, 77.0f, 92.0f,  0.0f,
                 0.0f, 82.0f, 87.0f, 97.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f};
  vtkm::Float32 cellvar[nCells] = {
                  0.0f,  1.0f,  2.0f,  3.0f,
                  4.0f,  5.0f,  6.0f,  7.0f,
                  8.0f,  9.0f, 10.0f, 11.0f,
                 12.0f, 13.0f, 14.0f, 15.0f,

                 16.0f, 17.0f, 18.0f, 19.0f,
                 20.0f, 21.0f, 22.0f, 23.0f,
                 24.0f, 25.0f, 26.0f, 27.0f,
                 28.0f, 29.0f, 30.0f, 31.0f,

                 32.0f, 33.0f, 34.0f, 35.0f,
                 36.0f, 37.0f, 38.0f, 39.0f,
                 40.0f, 41.0f, 42.0f, 43.0f,
                 44.0f, 45.0f, 46.0f, 47.0f,

                 48.0f, 49.0f, 50.0f, 51.0f,
                 52.0f, 53.0f, 54.0f, 55.0f,
                 56.0f, 57.0f, 58.0f, 59.0f,
                 60.0f, 61.0f, 62.0f, 63.0f};

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells, "cells");

  return dataSet;
}

//
// Create 2D explicit dataset
//
inline vtkm::cont::DataSet 
MakeFilterDataSet::Make2DExplicitFilterDataSet()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 16;
  const int nCells = 7;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(nVerts);

  coords[0]  = CoordType(0, 0, 0);
  coords[1]  = CoordType(1, 0, 0);
  coords[2]  = CoordType(2, 0, 0);
  coords[3]  = CoordType(3, 0, 0);
  coords[4]  = CoordType(0, 1, 0);
  coords[5]  = CoordType(1, 1, 0);
  coords[6]  = CoordType(2, 1, 0);
  coords[7]  = CoordType(3, 1, 0);
  coords[8]  = CoordType(0, 2, 0);
  coords[9]  = CoordType(1, 2, 0);
  coords[10] = CoordType(2, 2, 0);
  coords[11] = CoordType(3, 2, 0);
  coords[12] = CoordType(0, 3, 0);
  coords[13] = CoordType(3, 3, 0);
  coords[14] = CoordType(1, 4, 0);
  coords[15] = CoordType(2, 4, 0);

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(10);
  conn.push_back(9);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(9);
  conn.push_back(8);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(11);
  conn.push_back(10);

  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(6);
  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(13);
  conn.push_back(15);
  conn.push_back(14);
  conn.push_back(12);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates", "cells");

  // Field data
  vtkm::Float32 pointvar[nVerts] = {
               100.0f, 78.0f, 49.0f, 17.0f,
                94.0f, 71.0f, 47.0f, 33.0f,
                52.0f, 44.0f, 50.0f, 45.0f,
                 8.0f, 12.0f, 46.0f, 91.0f};
  vtkm::Float32 cellvar[nCells] = {
                 0.0f,  1.0f,  2.0f,  3.0f,
                 4.0f,  5.0f,  6.0f};

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells, "cells");

  return dataSet;
}

//
// Test 3D explicit dataset
//
inline vtkm::cont::DataSet 
MakeFilterDataSet::Make3DExplicitFilterDataSet()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 18;
  const int nCells = 4;
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(nVerts);

  coords[0]  = CoordType(0, 0, 0);
  coords[1]  = CoordType(1, 0, 0);
  coords[2]  = CoordType(2, 0, 0);
  coords[3]  = CoordType(3, 0, 0);
  coords[4]  = CoordType(0, 1, 0);
  coords[5]  = CoordType(1, 1, 0);
  coords[6]  = CoordType(2, 1, 0);
  coords[7]  = CoordType(2.5, 1.0, 0.0);
  coords[8]  = CoordType(0, 2, 0);
  coords[9]  = CoordType(1, 2, 0);
  coords[10]  = CoordType(0.5, 0.5, 1.0);
  coords[11]  = CoordType(1, 0, 1);
  coords[12]  = CoordType(2, 0, 1);
  coords[13]  = CoordType(3, 0, 1);
  coords[14]  = CoordType(1, 1, 1);
  coords[15]  = CoordType(2, 1, 1);
  coords[16]  = CoordType(2.5, 1.0, 1.0);
  coords[17]  = CoordType(0.5, 1.5, 1.0);

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);
  conn.push_back(10);

  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);
  conn.push_back(11);
  conn.push_back(12);
  conn.push_back(15);
  conn.push_back(14);

  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);
  conn.push_back(12);
  conn.push_back(13);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(9);
  conn.push_back(8);
  conn.push_back(17);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates", "cells");

  // Field data
  vtkm::Float32 pointvar[nVerts] = {
               100.0f, 78.0f, 49.0f, 17.0f,  1.0f,
                94.0f, 71.0f, 47.0f, 33.0f,  6.0f,
                52.0f, 44.0f, 50.0f, 45.0f, 48.0f,
                 8.0f, 12.0f, 46.0f};
  vtkm::Float32 cellvar[nCells] = {
                 0.0f,  1.0f,  2.0f,  3.0f};

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells, "cells");

  return dataSet;
}

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeFilterDataSet_h
