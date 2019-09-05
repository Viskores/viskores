//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_testing_MakeTestDataSet_h
#define vtk_m_cont_testing_MakeTestDataSet_h

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <vtkm/cont/testing/Testing.h>

#include <numeric>

namespace vtkm
{
namespace cont
{
namespace testing
{

class MakeTestDataSet
{
public:
  // 1D uniform datasets.
  vtkm::cont::DataSet Make1DUniformDataSet0();
  vtkm::cont::DataSet Make1DUniformDataSet1();
  vtkm::cont::DataSet Make1DUniformDataSet2();

  // 1D explicit datasets.
  vtkm::cont::DataSet Make1DExplicitDataSet0();

  // 2D uniform datasets.
  vtkm::cont::DataSet Make2DUniformDataSet0();
  vtkm::cont::DataSet Make2DUniformDataSet1();
  vtkm::cont::DataSet Make2DUniformDataSet2();

  // 3D uniform datasets.
  vtkm::cont::DataSet Make3DUniformDataSet0();
  vtkm::cont::DataSet Make3DUniformDataSet1();
  vtkm::cont::DataSet Make3DUniformDataSet2();
  vtkm::cont::DataSet Make3DUniformDataSet3(const vtkm::Id3 dims = vtkm::Id3(10));
  vtkm::cont::DataSet Make3DRegularDataSet0();
  vtkm::cont::DataSet Make3DRegularDataSet1();

  //2D rectilinear
  vtkm::cont::DataSet Make2DRectilinearDataSet0();

  //3D rectilinear
  vtkm::cont::DataSet Make3DRectilinearDataSet0();

  // 2D explicit datasets.
  vtkm::cont::DataSet Make2DExplicitDataSet0();

  // 3D explicit datasets.
  vtkm::cont::DataSet Make3DExplicitDataSet0();
  vtkm::cont::DataSet Make3DExplicitDataSet1();
  vtkm::cont::DataSet Make3DExplicitDataSet2();
  vtkm::cont::DataSet Make3DExplicitDataSet3();
  vtkm::cont::DataSet Make3DExplicitDataSet4();
  vtkm::cont::DataSet Make3DExplicitDataSet5();
  vtkm::cont::DataSet Make3DExplicitDataSet6();
  vtkm::cont::DataSet Make3DExplicitDataSet7();
  vtkm::cont::DataSet Make3DExplicitDataSet8();
  vtkm::cont::DataSet Make3DExplicitDataSetZoo();
  vtkm::cont::DataSet Make3DExplicitDataSetPolygonal();
  vtkm::cont::DataSet Make3DExplicitDataSetCowNose();
};

//Make a simple 1D dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet0()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  const vtkm::Id nVerts = 6;
  vtkm::cont::DataSet dataSet = dsb.Create(nVerts);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Float32 var[nVerts] = { -1.0f, .5f, -.2f, 1.7f, -.1f, .8f };
  constexpr vtkm::Float32 var2[nVerts] = { -1.1f, .7f, -.2f, 0.2f, -.1f, .4f };
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);
  dsf.AddPointField(dataSet, "pointvar2", var2, nVerts);

  return dataSet;
}

//Make another simple 1D dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet1()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  const vtkm::Id nVerts = 6;
  vtkm::cont::DataSet dataSet = dsb.Create(nVerts);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Float32 var[nVerts] = { 1.0e3f, 5.e5f, 2.e8f, 1.e10f, 2e12f, 3e15f };
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  return dataSet;
}


//Make a simple 1D, 16 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make1DUniformDataSet2()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id dims = 256;
  vtkm::cont::DataSet dataSet = dsb.Create(dims);

  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::Float64 pointvar[dims];
  constexpr vtkm::Float64 dx = vtkm::Float64(4.0 * vtkm::Pi()) / vtkm::Float64(dims - 1);

  vtkm::Id idx = 0;
  for (vtkm::Id x = 0; x < dims; ++x)
  {
    vtkm::Float64 cx = vtkm::Float64(x) * dx - 2.0 * vtkm::Pi();
    vtkm::Float64 cv = vtkm::Sin(cx);

    pointvar[idx] = cv;
    idx++;
  }

  dsf.AddPointField(dataSet, "pointvar", pointvar, dims);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make1DExplicitDataSet0()
{
  const int nVerts = 5;
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);
  coords[0] = CoordType(0.0f, 0.f, 0.f);
  coords[1] = CoordType(1.0f, 0.f, 0.f);
  coords[2] = CoordType(1.1f, 0.f, 0.f);
  coords[3] = CoordType(1.2f, 0.f, 0.f);
  coords[4] = CoordType(4.0f, 0.f, 0.f);

  // Each line connects two consecutive vertices
  std::vector<vtkm::Id> conn;
  for (int i = 0; i < nVerts - 1; i++)
  {
    conn.push_back(i);
    conn.push_back(i + 1);
  }

  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  dataSet = dsb.Create(coords, vtkm::CellShapeTagLine(), 2, conn, "coordinates");

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Float32 var[nVerts] = { -1.0f, .5f, -.2f, 1.7f, .8f };
  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  return dataSet;
}

//Make a simple 2D, 2 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet0()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id2 dimensions(3, 2);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Id nVerts = 6;
  constexpr vtkm::Float32 var[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.1f, 60.1f };

  dsf.AddPointField(dataSet, "pointvar", var, nVerts);

  constexpr vtkm::Float32 cellvar[2] = { 100.1f, 200.1f };
  dsf.AddCellField(dataSet, "cellvar", cellvar, 2);

  return dataSet;
}

//Make a simple 2D, 16 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet1()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id2 dimensions(5, 5);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Id nVerts = 25;
  constexpr vtkm::Id nCells = 16;
  constexpr vtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 1.0f,  94.0f, 71.0f,
                                               47.0f,  33.0f, 6.0f,  52.0f, 44.0f, 50.0f, 45.0f,
                                               48.0f,  8.0f,  12.0f, 46.0f, 91.0f, 43.0f, 0.0f,
                                               5.0f,   51.0f, 76.0f, 83.0f };
  constexpr vtkm::Float32 cellvar[nCells] = {
    0.0f, 1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
    8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f
  };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

//Make a simple 2D, 16 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make2DUniformDataSet2()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id2 dims(16, 16);
  vtkm::cont::DataSet dataSet = dsb.Create(dims);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Id nVerts = 256;
  vtkm::Float64 pointvar[nVerts];
  constexpr vtkm::Float64 dx = vtkm::Float64(4.0 * vtkm::Pi()) / vtkm::Float64(dims[0] - 1);
  constexpr vtkm::Float64 dy = vtkm::Float64(2.0 * vtkm::Pi()) / vtkm::Float64(dims[1] - 1);

  vtkm::Id idx = 0;
  for (vtkm::Id y = 0; y < dims[1]; ++y)
  {
    vtkm::Float64 cy = vtkm::Float64(y) * dy - vtkm::Pi();
    for (vtkm::Id x = 0; x < dims[0]; ++x)
    {
      vtkm::Float64 cx = vtkm::Float64(x) * dx - 2.0 * vtkm::Pi();
      vtkm::Float64 cv = vtkm::Sin(cx) + vtkm::Sin(cy) +
        2.0 * vtkm::Cos(vtkm::Sqrt((cx * cx) / 2.0 + cy * cy) / 0.75) +
        4.0 * vtkm::Cos(cx * cy / 4.0);

      pointvar[idx] = cv;
      idx++;
    }
  } // y

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);

  return dataSet;
}
//Make a simple 3D, 4 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet0()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id3 dimensions(3, 2, 3);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr int nVerts = 18;
  constexpr vtkm::Float32 vars[nVerts] = { 10.1f,  20.1f,  30.1f,  40.1f,  50.2f,  60.2f,
                                           70.2f,  80.2f,  90.3f,  100.3f, 110.3f, 120.3f,
                                           130.4f, 140.4f, 150.4f, 160.4f, 170.5f, 180.5f };

  //Set point and cell scalar
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);

  constexpr vtkm::Float32 cellvar[4] = { 100.1f, 100.2f, 100.3f, 100.4f };
  dsf.AddCellField(dataSet, "cellvar", cellvar, 4);

  return dataSet;
}

//Make a simple 3D, 64 cell uniform dataset.
inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet1()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  constexpr vtkm::Id3 dimensions(5, 5, 5);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Id nVerts = 125;
  constexpr vtkm::Id nCells = 64;
  constexpr vtkm::Float32 pointvar[nVerts] = {
    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f,
    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  99.0f, 90.0f, 85.0f, 0.0f, 0.0f, 95.0f, 80.0f,
    95.0f, 0.0f, 0.0f, 85.0f, 90.0f, 99.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  75.0f, 50.0f, 65.0f, 0.0f, 0.0f, 55.0f, 15.0f,
    45.0f, 0.0f, 0.0f, 60.0f, 40.0f, 70.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  97.0f, 87.0f, 82.0f, 0.0f, 0.0f, 92.0f, 77.0f,
    92.0f, 0.0f, 0.0f, 82.0f, 87.0f, 97.0f, 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,

    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f,  0.0f,
    0.0f,  0.0f, 0.0f, 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f
  };
  constexpr vtkm::Float32 cellvar[nCells] = {
    0.0f,  1.0f,  2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,
    8.0f,  9.0f,  10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,

    16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f,
    24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f,

    32.0f, 33.0f, 34.0f, 35.0f, 36.0f, 37.0f, 38.0f, 39.0f,
    40.0f, 41.0f, 42.0f, 43.0f, 44.0f, 45.0f, 46.0f, 47.0f,

    48.0f, 49.0f, 50.0f, 51.0f, 52.0f, 53.0f, 54.0f, 55.0f,
    56.0f, 57.0f, 58.0f, 59.0f, 60.0f, 61.0f, 62.0f, 63.0f
  };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet2()
{
  constexpr vtkm::Id base_size = 256;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id3 dimensions(base_size, base_size, base_size);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  constexpr vtkm::Id nVerts = base_size * base_size * base_size;
  vtkm::Float32* pointvar = new vtkm::Float32[nVerts];

  for (vtkm::Id z = 0; z < base_size; ++z)
    for (vtkm::Id y = 0; y < base_size; ++y)
      for (vtkm::Id x = 0; x < base_size; ++x)
      {
        std::size_t index = static_cast<std::size_t>(z * base_size * base_size + y * base_size + x);
        pointvar[index] = vtkm::Sqrt(vtkm::Float32(x * x + y * y + z * z));
      }

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);

  delete[] pointvar;

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DUniformDataSet3(const vtkm::Id3 dims)
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet dataSet = dsb.Create(dims);

  // add point scalar field
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];
  std::vector<vtkm::Float64> pointvar(static_cast<size_t>(numPoints));

  vtkm::Float64 dx = vtkm::Float64(4.0 * vtkm::Pi()) / vtkm::Float64(dims[0] - 1);
  vtkm::Float64 dy = vtkm::Float64(2.0 * vtkm::Pi()) / vtkm::Float64(dims[1] - 1);
  vtkm::Float64 dz = vtkm::Float64(3.0 * vtkm::Pi()) / vtkm::Float64(dims[2] - 1);

  vtkm::Id idx = 0;
  for (vtkm::Id z = 0; z < dims[2]; ++z)
  {
    vtkm::Float64 cz = vtkm::Float64(z) * dz - 1.5 * vtkm::Pi();
    for (vtkm::Id y = 0; y < dims[1]; ++y)
    {
      vtkm::Float64 cy = vtkm::Float64(y) * dy - vtkm::Pi();
      for (vtkm::Id x = 0; x < dims[0]; ++x)
      {
        vtkm::Float64 cx = vtkm::Float64(x) * dx - 2.0 * vtkm::Pi();
        vtkm::Float64 cv = vtkm::Sin(cx) + vtkm::Sin(cy) +
          2.0 * vtkm::Cos(vtkm::Sqrt((cx * cx) / 2.0 + cy * cy) / 0.75) +
          4.0 * vtkm::Cos(cx * cy / 4.0);

        if (dims[2] > 1)
        {
          cv += vtkm::Sin(cz) + 1.5 * vtkm::Cos(vtkm::Sqrt(cx * cx + cy * cy + cz * cz) / 0.75);
        }
        pointvar[static_cast<size_t>(idx)] = cv;
        idx++;
      }
    } // y
  }   // z

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", pointvar);

  vtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1);
  dsf.AddCellField(
    dataSet,
    "cellvar",
    vtkm::cont::make_ArrayHandleCounting(vtkm::Float64(0), vtkm::Float64(1), numCells));

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make2DRectilinearDataSet0()
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
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DRegularDataSet0()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 18;
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vtkm::Id3(3, 2, 3));
  vtkm::Float32 vars[nVerts] = { 10.1f,  20.1f,  30.1f,  40.1f,  50.2f,  60.2f,
                                 70.2f,  80.2f,  90.3f,  100.3f, 110.3f, 120.3f,
                                 130.4f, 140.4f, 150.4f, 160.4f, 170.5f, 180.5f };

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  vtkm::Float32 cellvar[4] = { 100.1f, 100.2f, 100.3f, 100.4f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, 4, vtkm::CopyFlag::On));

  static constexpr vtkm::IdComponent dim = 3;
  vtkm::cont::CellSetStructured<dim> cellSet;
  cellSet.SetPointDimensions(vtkm::make_Vec(3, 2, 3));
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DRegularDataSet1()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 8;
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vtkm::Id3(2, 2, 2));
  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.1f, 40.1f, 50.2f, 60.2f, 70.2f, 80.2f };

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  vtkm::Float32 cellvar[1] = { 100.1f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, 1, vtkm::CopyFlag::On));

  static constexpr vtkm::IdComponent dim = 3;
  vtkm::cont::CellSetStructured<dim> cellSet;
  cellSet.SetPointDimensions(vtkm::make_Vec(2, 2, 2));
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DRectilinearDataSet0()
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
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

// Make a 2D explicit dataset
inline vtkm::cont::DataSet MakeTestDataSet::Make2DExplicitDataSet0()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 16;
  const int nCells = 7;
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(2, 0, 0);
  coords[3] = CoordType(3, 0, 0);
  coords[4] = CoordType(0, 1, 0);
  coords[5] = CoordType(1, 1, 0);
  coords[6] = CoordType(2, 1, 0);
  coords[7] = CoordType(3, 1, 0);
  coords[8] = CoordType(0, 2, 0);
  coords[9] = CoordType(1, 2, 0);
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
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  vtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 33.0f,
                                     52.0f,  44.0f, 50.0f, 45.0f, 8.0f,  12.0f, 46.0f, 91.0f };
  vtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet0()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 5;
  using CoordType = vtkm::Vec3f_32;
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
  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };
  vtkm::Float32 cellvar[2] = { 100.1f, 100.2f };

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, 2);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet1()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 5;
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords(nVerts);

  coords[0] = CoordType(0, 0, 0);
  coords[1] = CoordType(1, 0, 0);
  coords[2] = CoordType(1, 1, 0);
  coords[3] = CoordType(2, 1, 0);
  coords[4] = CoordType(2, 2, 0);
  CoordType coordinates[nVerts] = { CoordType(0, 0, 0),
                                    CoordType(1, 0, 0),
                                    CoordType(1, 1, 0),
                                    CoordType(2, 1, 0),
                                    CoordType(2, 2, 0) };
  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f };

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));
  vtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(2, 7);
  cellSet.AddCell(vtkm::CELL_SHAPE_TRIANGLE, 3, make_Vec<vtkm::Id>(0, 1, 2));
  cellSet.AddCell(vtkm::CELL_SHAPE_QUAD, 4, make_Vec<vtkm::Id>(2, 1, 3, 4));
  cellSet.CompleteAddingCells(nVerts);
  dataSet.SetCellSet(cellSet);

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = { 100.1f, 100.2f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, 2, vtkm::CopyFlag::On));

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet2()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 8;
  using CoordType = vtkm::Vec3f_32;
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
  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f, 70.2f, 80.3f };

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = { 100.1f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, 1, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet;
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
  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet4()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 12;
  using CoordType = vtkm::Vec3f_32;
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
  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f,
                                 70.2f, 80.3f, 90.f,  10.f,  11.f,  12.f };

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = { 100.1f, 110.f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, 2, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet;
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
  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet3()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 4;
  using CoordType = vtkm::Vec3f_32;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0), CoordType(1, 0, 0), CoordType(1, 0, 1), CoordType(0, 1, 0)
  };
  vtkm::Float32 vars[nVerts] = { 10.1f, 10.1f, 10.2f, 30.2f };

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  vtkm::Float32 cellvar[2] = { 100.1f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, 1, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet;
  vtkm::Id4 ids;
  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 2;
  ids[3] = 3;

  cellSet.PrepareToAddCells(1, 4);
  cellSet.AddCell(vtkm::CELL_SHAPE_TETRA, 4, ids);
  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet5()
{
  vtkm::cont::DataSet dataSet;

  const int nVerts = 11;
  using CoordType = vtkm::Vec3f_32;
  CoordType coordinates[nVerts] = {
    CoordType(0, 0, 0),     //0
    CoordType(1, 0, 0),     //1
    CoordType(1, 0, 1),     //2
    CoordType(0, 0, 1),     //3
    CoordType(0, 1, 0),     //4
    CoordType(1, 1, 0),     //5
    CoordType(1, 1, 1),     //6
    CoordType(0, 1, 1),     //7
    CoordType(2, 0.5, 0.5), //8
    CoordType(0, 2, 0),     //9
    CoordType(1, 2, 0)      //10
  };
  vtkm::Float32 vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.2f,
                                 70.2f, 80.3f, 90.f,  10.f,  11.f };

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));

  //Set point scalar
  dataSet.AddField(make_Field(
    "pointvar", vtkm::cont::Field::Association::POINTS, vars, nVerts, vtkm::CopyFlag::On));

  //Set cell scalar
  const int nCells = 4;
  vtkm::Float32 cellvar[nCells] = { 100.1f, 110.f, 120.2f, 130.5f };
  dataSet.AddField(make_Field(
    "cellvar", vtkm::cont::Field::Association::CELL_SET, cellvar, nCells, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet;
  vtkm::Vec<vtkm::Id, 8> ids;

  cellSet.PrepareToAddCells(nCells, 23);

  ids[0] = 0;
  ids[1] = 1;
  ids[2] = 5;
  ids[3] = 4;
  ids[4] = 3;
  ids[5] = 2;
  ids[6] = 6;
  ids[7] = 7;
  cellSet.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON, 8, ids);

  ids[0] = 1;
  ids[1] = 5;
  ids[2] = 6;
  ids[3] = 2;
  ids[4] = 8;
  cellSet.AddCell(vtkm::CELL_SHAPE_PYRAMID, 5, ids);

  ids[0] = 5;
  ids[1] = 8;
  ids[2] = 10;
  ids[3] = 6;
  cellSet.AddCell(vtkm::CELL_SHAPE_TETRA, 4, ids);

  ids[0] = 4;
  ids[1] = 7;
  ids[2] = 9;
  ids[3] = 5;
  ids[4] = 6;
  ids[5] = 10;
  cellSet.AddCell(vtkm::CELL_SHAPE_WEDGE, 6, ids);

  cellSet.CompleteAddingCells(nVerts);

  //todo this need to be a reference/shared_ptr style class
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet6()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 8;
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 10.0f, 10.0f, 10.0f },       { 5.0f, 5.0f, 5.0f },
                                    { 0.0f, 0.0f, 2.0f },          { 0.0f, 0.0f, -2.0f } };

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(0);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(4);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(6);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(0);
  conn.push_back(7);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  vtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 57.0f };
  vtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetZoo()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  constexpr int nVerts = 30;
  constexpr int nCells = 25;
  using CoordType = vtkm::Vec3f_32;

  std::vector<CoordType> coords =

    { { 0.00f, 0.00f, 0.00f }, { 1.00f, 0.00f, 0.00f }, { 2.00f, 0.00f, 0.00f },
      { 0.00f, 0.00f, 1.00f }, { 1.00f, 0.00f, 1.00f }, { 2.00f, 0.00f, 1.00f },
      { 0.00f, 1.00f, 0.00f }, { 1.00f, 1.00f, 0.00f }, { 2.00f, 1.00f, 0.00f },
      { 0.00f, 1.00f, 1.00f }, { 1.00f, 1.00f, 1.00f }, { 2.00f, 1.00f, 1.00f },
      { 0.00f, 2.00f, 0.00f }, { 1.00f, 2.00f, 0.00f }, { 2.00f, 2.00f, 0.00f },
      { 0.00f, 2.00f, 1.00f }, { 1.00f, 2.00f, 1.00f }, { 2.00f, 2.00f, 1.00f },
      { 1.00f, 3.00f, 1.00f }, { 2.75f, 0.00f, 1.00f }, { 3.00f, 0.00f, 0.75f },
      { 3.00f, 0.25f, 1.00f }, { 3.00f, 1.00f, 1.00f }, { 3.00f, 1.00f, 0.00f },
      { 2.57f, 2.00f, 1.00f }, { 3.00f, 1.75f, 1.00f }, { 3.00f, 1.75f, 0.75f },
      { 3.00f, 0.00f, 0.00f }, { 2.57f, 0.42f, 0.57f }, { 2.59f, 1.43f, 0.71f } };

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(4);
  conn.push_back(1);
  conn.push_back(6);
  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(7);

  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(1);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(2);
  conn.push_back(7);
  conn.push_back(10);
  conn.push_back(11);
  conn.push_back(8);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(23);
  conn.push_back(26);
  conn.push_back(24);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(24);
  conn.push_back(26);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(8);
  conn.push_back(17);
  conn.push_back(11);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(17);
  conn.push_back(24);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(24);
  conn.push_back(17);
  conn.push_back(8);
  conn.push_back(23);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(23);
  conn.push_back(8);
  conn.push_back(11);
  conn.push_back(22);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(25);
  conn.push_back(22);
  conn.push_back(11);
  conn.push_back(17);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(26);
  conn.push_back(23);
  conn.push_back(22);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(23);
  conn.push_back(8);
  conn.push_back(2);
  conn.push_back(27);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(22);
  conn.push_back(11);
  conn.push_back(8);
  conn.push_back(23);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(11);
  conn.push_back(5);
  conn.push_back(2);
  conn.push_back(8);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(21);
  conn.push_back(19);
  conn.push_back(5);
  conn.push_back(11);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(11);
  conn.push_back(22);
  conn.push_back(21);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(5);
  conn.push_back(19);
  conn.push_back(20);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(23);
  conn.push_back(27);
  conn.push_back(20);
  conn.push_back(21);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(20);
  conn.push_back(27);
  conn.push_back(2);
  conn.push_back(5);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(19);
  conn.push_back(21);
  conn.push_back(20);
  conn.push_back(28);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(7);
  conn.push_back(6);
  conn.push_back(12);
  conn.push_back(13);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(6);
  conn.push_back(9);
  conn.push_back(15);
  conn.push_back(12);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(10);
  conn.push_back(9);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(12);
  conn.push_back(15);
  conn.push_back(16);
  conn.push_back(18);

  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(8);
  conn.push_back(14);
  conn.push_back(17);
  conn.push_back(7);
  conn.push_back(13);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(11);
  conn.push_back(8);
  conn.push_back(17);
  conn.push_back(10);
  conn.push_back(7);
  conn.push_back(16);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  vtkm::Float32 pointvar[nVerts] =

    { 4.0,  5.0f, 9.5f, 5.5f, 6.0f, 9.5f, 5.0f, 5.5f, 5.7f, 6.5f, 6.4f, 6.9f, 6.6f, 6.1f, 7.1f,
      7.2f, 7.3f, 7.4f, 9.1f, 9.2f, 9.3f, 5.4f, 9.5f, 9.6f, 6.7f, 9.8f, 6.0f, 4.3f, 4.9f, 4.1f };

  vtkm::Float32 cellvar[nCells] =

    { 4.0f, 5.0f, 9.5f, 5.5f, 6.0f, 9.5f, 5.0f, 5.5f, 5.7f, 6.5f, 6.4f, 6.9f, 6.6f,
      6.1f, 7.1f, 7.2f, 7.3f, 7.4f, 9.1f, 9.2f, 9.3f, 5.4f, 9.5f, 9.6f, 6.7f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet7()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 8;

  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 10.0f, 10.0f, 10.0f },       { 5.0f, 5.0f, 5.0f },
                                    { 0.0f, 0.0f, 2.0f },          { 0.0f, 0.0f, -2.0f } };

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;


  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(0);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(2);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(3);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(4);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(6);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(7);


  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  vtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 10.f, 20.f, 33.f, 52.f };
  vtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSet8()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 10;
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 10.0f, 10.0f, 10.0f },       { 5.0f, 5.0f, 5.0f },
                                    { 0.0f, 0.0f, 2.0f },          { 0.0f, 0.0f, -2.0f } };

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  //I need two triangles because the leaf needs four nodes otherwise segfault?
  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(0);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(1);
  conn.push_back(2);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(3);
  conn.push_back(4);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(4);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(5);
  conn.push_back(6);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(6);
  conn.push_back(7);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(5);
  conn.push_back(4);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  vtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 57.0f };
  vtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);
  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetPolygonal()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSetFieldAdd dsf;

  // Coordinates
  const int nVerts = 8;
  const int nCells = 8;
  using CoordType = vtkm::Vec3f_32;
  std::vector<CoordType> coords = { { -0.707f, -0.354f, -0.354f }, { 0.000f, -0.854f, 0.146f },
                                    { 0.000f, -0.146f, 0.854f },   { -0.707f, 0.354f, 0.354f },
                                    { 0.000f, 0.146f, -0.854f },   { 0.000f, 0.854f, -0.146f },
                                    { 0.707f, 0.354f, 0.354f },    { 0.707f, -0.354f, -0.354f } };

  // Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(3);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(3);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(4);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(4);
  conn.push_back(7);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(4);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(5);
  conn.push_back(4);

  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(4);
  conn.push_back(1);
  conn.push_back(7);
  conn.push_back(6);
  conn.push_back(2);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  // Field data
  vtkm::Float32 pointvar[nVerts] = { 100.0f, 78.0f, 49.0f, 17.0f, 94.0f, 71.0f, 47.0f, 33.0f };
  vtkm::Float32 cellvar[nCells] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };

  dsf.AddPointField(dataSet, "pointvar", pointvar, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitDataSetCowNose()
{
  // prepare data array
  const int nVerts = 17;
  using CoordType = vtkm::Vec3f_64;
  CoordType coordinates[nVerts] = {
    CoordType(0.0480879, 0.151874, 0.107334),     CoordType(0.0293568, 0.245532, 0.125337),
    CoordType(0.0224398, 0.246495, 0.1351),       CoordType(0.0180085, 0.20436, 0.145316),
    CoordType(0.0307091, 0.152142, 0.0539249),    CoordType(0.0270341, 0.242992, 0.107567),
    CoordType(0.000684071, 0.00272505, 0.175648), CoordType(0.00946217, 0.077227, 0.187097),
    CoordType(-0.000168991, 0.0692243, 0.200755), CoordType(-0.000129414, 0.00247137, 0.176561),
    CoordType(0.0174172, 0.137124, 0.124553),     CoordType(0.00325994, 0.0797155, 0.184912),
    CoordType(0.00191765, 0.00589327, 0.16608),   CoordType(0.0174716, 0.0501928, 0.0930275),
    CoordType(0.0242103, 0.250062, 0.126256),     CoordType(0.0108188, 0.152774, 0.167914),
    CoordType(5.41687e-05, 0.00137834, 0.175119)
  };
  const int connectivitySize = 57;
  vtkm::Id pointId[connectivitySize] = { 0, 1, 3,  2, 3,  1, 4,  5,  0,  1, 0,  5,  7,  8,  6,
                                         9, 6, 8,  0, 10, 7, 11, 7,  10, 0, 6,  13, 12, 13, 6,
                                         1, 5, 14, 1, 14, 2, 0,  3,  15, 0, 13, 4,  6,  16, 12,
                                         6, 9, 16, 7, 11, 8, 0,  15, 10, 7, 6,  0 };

  // create DataSet
  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  connectivity.Allocate(connectivitySize);

  for (vtkm::Id i = 0; i < connectivitySize; ++i)
  {
    connectivity.GetPortalControl().Set(i, pointId[i]);
  }
  vtkm::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(nVerts, vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);
  dataSet.SetCellSet(cellSet);

  std::vector<vtkm::Float32> pointvar(nVerts);
  std::iota(pointvar.begin(), pointvar.end(), 15.f);
  std::vector<vtkm::Float32> cellvar(connectivitySize / 3);
  std::iota(cellvar.begin(), cellvar.end(), 132.f);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> pointvec;
  pointvec.Allocate(nVerts);
  SetPortal(pointvec.GetPortalControl());

  vtkm::cont::ArrayHandle<vtkm::Vec3f> cellvec;
  cellvec.Allocate(connectivitySize / 3);
  SetPortal(cellvec.GetPortalControl());

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", pointvar);
  dsf.AddCellField(dataSet, "cellvar", cellvar);
  dsf.AddPointField(dataSet, "point_vectors", pointvec);
  dsf.AddCellField(dataSet, "cell_vectors", cellvec);

  return dataSet;
}
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_MakeTestDataSet_h
