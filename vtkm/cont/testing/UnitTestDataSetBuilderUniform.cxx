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
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <random>
#include <time.h>
#include <vector>

namespace DataSetBuilderUniformNamespace
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
  VTKM_TEST_ASSERT(test_equal(bounds, res), "Bounds of coordinates do not match");
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
vtkm::Range FillMethod(vtkm::IdComponent method, vtkm::Id dimensionSize, T& origin, T& spacing)
{
  switch (method)
  {
    case 0:
      origin = 0;
      spacing = 1;
      break;
    case 1:
      origin = 0;
      spacing = static_cast<T>(1.0 / static_cast<double>(dimensionSize));
      break;
    case 2:
      origin = 0;
      spacing = 2;
      break;
    case 3:
      origin = static_cast<T>(-(dimensionSize - 1));
      spacing = 1;
      break;
    case 4:
      origin = static_cast<T>(2.780941);
      spacing = static_cast<T>(182.381901);
      break;
    default:
      origin = 0;
      spacing = 0;
      break;
  }

  return vtkm::Range(origin, origin + static_cast<T>(dimensionSize - 1) * spacing);
}

vtkm::Range& GetRangeByIndex(vtkm::Bounds& bounds, int comp)
{
  VTKM_ASSERT(comp >= 0 && comp < 3);
  switch (comp)
  {
    case 0:
      return bounds.X;
    case 1:
      return bounds.Y;
    default:
      return bounds.Z;
  }
}

template <typename T>
void UniformTests()
{
  const vtkm::Id NUM_TRIALS = 10;
  const vtkm::Id MAX_DIM_SIZE = 20;
  const vtkm::Id NUM_FILL_METHODS = 5;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSetFieldAdd dsf;

  std::uniform_int_distribution<vtkm::Id> randomDim(2, MAX_DIM_SIZE);
  std::uniform_int_distribution<vtkm::IdComponent> randomFill(0, NUM_FILL_METHODS - 1);
  std::uniform_int_distribution<vtkm::IdComponent> randomAxis(0, 2);

  for (vtkm::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    std::cout << "Trial " << trial << std::endl;

    vtkm::Id3 dimensions(
      randomDim(g_RandomGenerator), randomDim(g_RandomGenerator), randomDim(g_RandomGenerator));

    vtkm::IdComponent fillMethodX = randomFill(g_RandomGenerator);
    vtkm::IdComponent fillMethodY = randomFill(g_RandomGenerator);
    vtkm::IdComponent fillMethodZ = randomFill(g_RandomGenerator);
    std::cout << "Fill methods: [" << fillMethodX << "," << fillMethodY << "," << fillMethodZ << "]"
              << std::endl;

    vtkm::Vec<T, 3> origin;
    vtkm::Vec<T, 3> spacing;
    vtkm::Range ranges[3];
    ranges[0] = FillMethod(fillMethodX, dimensions[0], origin[0], spacing[0]);
    ranges[1] = FillMethod(fillMethodY, dimensions[1], origin[1], spacing[1]);
    ranges[2] = FillMethod(fillMethodZ, dimensions[2], origin[2], spacing[2]);

    std::cout << "3D cellset" << std::endl;
    {
      vtkm::Id3 dims = dimensions;
      vtkm::Bounds bounds(ranges[0], ranges[1], ranges[2]);

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << origin << std::endl;
      std::cout << "\tspacing: " << spacing << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      vtkm::Id numPoints = dims[0] * dims[1] * dims[2];
      vtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1) * (dims[2] - 1);

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      vtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, origin, spacing);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);
    }

    std::cout << "2D cellset, 2D parameters" << std::endl;
    {
      vtkm::Id2 dims(dimensions[0], dimensions[1]);
      vtkm::Bounds bounds(ranges[0], ranges[1], vtkm::Range(0, 0));
      vtkm::Vec<T, 2> org(origin[0], origin[1]);
      vtkm::Vec<T, 2> spc(spacing[0], spacing[1]);

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << org << std::endl;
      std::cout << "\tspacing: " << spc << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      vtkm::Id numPoints = dims[0] * dims[1];
      vtkm::Id numCells = (dims[0] - 1) * (dims[1] - 1);

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      vtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, org, spc);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);
    }

    std::cout << "2D cellset, 3D parameters" << std::endl;
    {
      vtkm::Id3 dims = dimensions;
      vtkm::Bounds bounds(ranges[0], ranges[1], ranges[2]);

      int x = randomAxis(g_RandomGenerator);
      dims[x] = 1;
      GetRangeByIndex(bounds, x).Max = ranges[x].Min;

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << origin << std::endl;
      std::cout << "\tspacing: " << spacing << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      vtkm::Id numPoints = dims[(x + 1) % 3] * dims[(x + 2) % 3];
      vtkm::Id numCells = (dims[(x + 1) % 3] - 1) * (dims[(x + 2) % 3] - 1);

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      vtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, origin, spacing);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);
    }

    std::cout << "1D cellset, 1D parameters" << std::endl;
    {
      vtkm::Bounds bounds(ranges[0], vtkm::Range(0, 0), vtkm::Range(0, 0));

      std::cout << "\tdimensions: " << dimensions[0] << std::endl;
      std::cout << "\toriging: " << origin[0] << std::endl;
      std::cout << "\tspacing: " << spacing[0] << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      vtkm::Id numPoints = dimensions[0];
      vtkm::Id numCells = dimensions[0] - 1;

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      vtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dimensions[0], origin[0], spacing[0]);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
    }

    std::cout << "1D cellset, 2D parameters" << std::endl;
    {
      vtkm::Id2 dims(dimensions[0], dimensions[1]);
      vtkm::Bounds bounds(ranges[0], ranges[1], vtkm::Range(0, 0));
      vtkm::Vec<T, 2> org(origin[0], origin[1]);
      vtkm::Vec<T, 2> spc(spacing[0], spacing[1]);

      int x = randomAxis(g_RandomGenerator) % 2;
      dims[x] = 1;
      GetRangeByIndex(bounds, x).Max = ranges[x].Min;

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << org << std::endl;
      std::cout << "\tspacing: " << spc << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      vtkm::Id numPoints = dims[(x + 1) % 2];
      vtkm::Id numCells = dims[(x + 1) % 2] - 1;

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      vtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, org, spc);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
    }

    std::cout << "1D cellset, 3D parameters" << std::endl;
    {
      vtkm::Id3 dims = dimensions;
      vtkm::Bounds bounds(ranges[0], ranges[1], ranges[2]);

      int x = randomAxis(g_RandomGenerator);
      int x1 = (x + 1) % 3;
      int x2 = (x + 2) % 3;
      dims[x1] = dims[x2] = 1;
      GetRangeByIndex(bounds, x1).Max = ranges[x1].Min;
      GetRangeByIndex(bounds, x2).Max = ranges[x2].Min;

      std::cout << "\tdimensions: " << dims << std::endl;
      std::cout << "\toriging: " << origin << std::endl;
      std::cout << "\tspacing: " << spacing << std::endl;
      std::cout << "\tbounds: " << bounds << std::endl;

      vtkm::Id numPoints = dims[x];
      vtkm::Id numCells = dims[x] - 1;

      std::vector<T> pointvar(static_cast<unsigned long>(numPoints));
      std::iota(pointvar.begin(), pointvar.end(), T(1.1));
      std::vector<T> cellvar(static_cast<unsigned long>(numCells));
      std::iota(cellvar.begin(), cellvar.end(), T(1.1));

      vtkm::cont::DataSet dataSet;
      dataSet = dataSetBuilder.Create(dims, origin, spacing);
      dsf.AddPointField(dataSet, "pointvar", pointvar);
      dsf.AddCellField(dataSet, "cellvar", cellvar);

      ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
    }
  }
}

void TestDataSetBuilderUniform()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  UniformTests<vtkm::Float32>();
  std::cout << "======== Float64 ==========================" << std::endl;
  UniformTests<vtkm::Float64>();
}

} // namespace DataSetBuilderUniformNamespace

int UnitTestDataSetBuilderUniform(int argc, char* argv[])
{
  using namespace DataSetBuilderUniformNamespace;
  return vtkm::cont::testing::Testing::Run(TestDataSetBuilderUniform, argc, argv);
}
