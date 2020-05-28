//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_testing_MakeTestDataSet_h
#define vtk_m_filter_testing_MakeTestDataSet_h

#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include <vtkm/cont/testing/Testing.h>

#include <numeric>

namespace vtkm
{
namespace filter
{
namespace testing
{

class MakeTestDataSet
{
public:
  // 3D explicit datasets
  vtkm::cont::DataSet Make3DExplicitSimpleCube();

  // Special test datasets
  vtkm::cont::DataSet MakePointTransformTestDataSet();
  vtkm::cont::DataSet MakeStreamlineTestDataSet(const vtkm::Id3& dims, const vtkm::Vec3f& vec);
};

inline vtkm::cont::DataSet MakeTestDataSet::MakePointTransformTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec3f> coordinates;
  const vtkm::Id dim = 5;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    vtkm::FloatDefault z =
      static_cast<vtkm::FloatDefault>(j) / static_cast<vtkm::FloatDefault>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      vtkm::FloatDefault x =
        static_cast<vtkm::FloatDefault>(i) / static_cast<vtkm::FloatDefault>(dim - 1);
      vtkm::FloatDefault y = (x * x + z * z) / 2.0f;
      coordinates.push_back(vtkm::make_Vec(x, y, z));
    }
  }

  vtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet;
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (vtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (vtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(vtkm::CELL_SHAPE_QUAD,
                      4,
                      vtkm::make_Vec<vtkm::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(vtkm::Id(coordinates.size()));

  dataSet.SetCellSet(cellSet);
  return dataSet;
}

inline vtkm::cont::DataSet MakeTestDataSet::MakeStreamlineTestDataSet(const vtkm::Id3& dims,
                                                                      const vtkm::Vec3f& vec)
{
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<vtkm::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vec;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims);
  ds.AddPointField("vector", vectorField);

  return ds;
}

inline vtkm::cont::DataSet MakeTestDataSet::Make3DExplicitSimpleCube()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 8;
  const int nCells = 6;
  using CoordType = vtkm::Vec3f;
  std::vector<CoordType> coords = {
    CoordType(0, 0, 0), // 0
    CoordType(1, 0, 0), // 1
    CoordType(1, 0, 1), // 2
    CoordType(0, 0, 1), // 3
    CoordType(0, 1, 0), // 4
    CoordType(1, 1, 0), // 5
    CoordType(1, 1, 1), // 6
    CoordType(0, 1, 1)  // 7
  };

  //Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numIndices;
  for (size_t i = 0; i < 6; i++)
  {
    shapes.push_back(vtkm::CELL_SHAPE_QUAD);
    numIndices.push_back(4);
  }

  std::vector<vtkm::Id> conn;
  // Down face
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);
  conn.push_back(4);
  // Right face
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);
  // Top face
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);
  conn.push_back(6);
  // Left face
  conn.push_back(3);
  conn.push_back(0);
  conn.push_back(4);
  conn.push_back(7);
  // Front face
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);
  // Back face
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(1);
  //Create the dataset.
  dataSet = dsb.Create(coords, shapes, numIndices, conn, "coordinates");

  vtkm::FloatDefault vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f, 70.3f, 80.3f };
  vtkm::FloatDefault cellvar[nCells] = { 100.1f, 200.2f, 300.3f, 400.4f, 500.5f, 600.6f };

  dataSet.AddPointField("pointvar", vars, nVerts);
  dataSet.AddCellField("cellvar", cellvar, nCells);

  return dataSet;
}

}
}
} // namespace vtkm::filter::testing

#endif //vtk_m_filter_testing_MakeTestDataSet_h
