//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSetBuilderCurvilinear.h>
#include <vtkm/cont/testing/Testing.h>

#include <random>
#include <time.h>
#include <vector>

namespace DataSetBuilderCurvilinearNamespace
{

std::mt19937 g_RandomGenerator;

void ValidateDataSet(const vtkm::cont::DataSet& ds,
                     int dim,
                     vtkm::Id numPoints,
                     vtkm::Id numCells,
                     vtkm::Bounds bounds)
{
  //Verify basics..

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Wrong number of fields.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1, "Wrong number of coordinate systems.");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numPoints, "Wrong number of coordinates.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == numCells, "Wrong number of cells.");

  // test various field-getting methods and associations
  try
  {
    ds.GetCellField("cellvar");
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
  }

  try
  {
    ds.GetPointField("pointvar");
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  //Make sure bounds are correct.
  vtkm::Bounds res = ds.GetCoordinateSystem().GetBounds();
  VTKM_TEST_ASSERT(bounds.Contains(vtkm::Vec3f_64(res.X.Min, res.Y.Min, res.Z.Min)) &&
                     bounds.Contains(vtkm::Vec3f_64(res.X.Max, res.Y.Max, res.Z.Max)),
                   test_equal(bounds, res),
                   "Bounds of coordinates do not match");
  if (dim == 1)
  {
    vtkm::cont::CellSetStructured<1> cellSet;
    ds.GetCellSet().CopyTo(cellSet);
    vtkm::IdComponent shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_LINE, "Wrong element type");
  }
  else if (dim == 2)
  {
    vtkm::cont::CellSetStructured<2> cellSet;
    ds.GetCellSet().CopyTo(cellSet);
    vtkm::IdComponent shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_QUAD, "Wrong element type");
  }
  else if (dim == 3)
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    ds.GetCellSet().CopyTo(cellSet);
    vtkm::IdComponent shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_HEXAHEDRON, "Wrong element type");
  }
}

template <typename T>
void AddFields(vtkm::cont::DataSet& ds, vtkm::Id numPoints, vtkm::Id numCells)
{
  //Add some fields.
  std::vector<T> cellVar(numCells, 0), pointVar(numPoints, 1);
  ds.AddPointField("pointvar", pointVar);
  ds.AddCellField("cellvar", cellVar);
}

template <typename T>
void CurvilinearTests()
{
  constexpr T minReal = -10, maxReal = 10;
  std::uniform_real_distribution<T> randomVal(minReal, maxReal);
  std::uniform_int_distribution<vtkm::Id> randomDim(2, 20);

  vtkm::Bounds bounds(minReal, maxReal, minReal, maxReal, minReal, maxReal);

  for (int i = 0; i < 10; i++)
  {
    vtkm::Id3 dims(
      randomDim(g_RandomGenerator), randomDim(g_RandomGenerator), randomDim(g_RandomGenerator));

    vtkm::Id numPoints = 1;
    vtkm::Id numCells = 1;

    for (int ndim = 0; ndim < 3; ndim++)
    {
      numPoints *= dims[ndim];
      numCells *= (dims[ndim] - 1);

      std::vector<T> x, y, z;
      for (vtkm::Id j = 0; j < numPoints; j++)
      {
        x.push_back(randomVal(g_RandomGenerator));
        y.push_back(randomVal(g_RandomGenerator));
        z.push_back(randomVal(g_RandomGenerator));
      }

      //test 3d
      if (ndim == 2)
      {
        auto ds = vtkm::cont::DataSetBuilderCurvilinear::Create(x, y, z, dims);
        AddFields<T>(ds, numPoints, numCells);
        ValidateDataSet(ds, 3, numPoints, numCells, bounds);
      }
      //test 2d
      else if (ndim == 1)
      {
        auto ds = vtkm::cont::DataSetBuilderCurvilinear::Create(x, y, { dims[0], dims[1] });
        AddFields<T>(ds, numPoints, numCells);
        ValidateDataSet(ds, 2, numPoints, numCells, bounds);
      }
      //test 1d
      else if (ndim == 0)
      {
        auto ds = vtkm::cont::DataSetBuilderCurvilinear::Create(x);
        AddFields<T>(ds, numPoints, numCells);
        ValidateDataSet(ds, 1, numPoints, numCells, bounds);
      }
    }
  }
}

void TestDataSetBuilderCurvilinear()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(time(nullptr));
  g_RandomGenerator.seed(seed);

  CurvilinearTests<vtkm::Float32>();
  CurvilinearTests<vtkm::Float64>();
}

} // namespace DataSetBuilderCurvilinearNamespace

int UnitTestDataSetBuilderCurvilinear(int argc, char* argv[])
{
  using namespace DataSetBuilderCurvilinearNamespace;
  return vtkm::cont::testing::Testing::Run(TestDataSetBuilderCurvilinear, argc, argv);
}
