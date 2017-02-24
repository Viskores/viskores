//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <time.h>
#include <random>
#include <vector>

namespace DataSetBuilderUniformNamespace {

std::mt19937 g_RandomGenerator;

void ValidateDataSet(const vtkm::cont::DataSet &ds,
                     int dim,
                     vtkm::Id numPoints, vtkm::Id numCells,
                     vtkm::Bounds bounds)
{
  //Verify basics..
  VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1,
                   "Wrong number of cell sets.");
  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2,
                   "Wrong number of fields.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems.");
  VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == numPoints,
                   "Wrong number of coordinates.");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == numCells,
                   "Wrong number of cells.");

  // test various field-getting methods and associations
  try
  {
    ds.GetField("cellvar", vtkm::cont::Field::ASSOC_CELL_SET);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with ASSOC_CELL_SET.");
  }

  try
  {
    ds.GetField("pointvar", vtkm::cont::Field::ASSOC_POINTS);
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  //Make sure bounds are correct.
  vtkm::Bounds res = ds.GetCoordinateSystem().GetBounds();
  VTKM_TEST_ASSERT(test_equal(bounds, res),
                   "Bounds of coordinates do not match");
  if (dim == 1)
  {
    vtkm::cont::CellSetStructured<1> cellSet;
    ds.GetCellSet(0).CopyTo(cellSet);
    vtkm::IdComponent shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_LINE, "Wrong element type");
  }
  else if (dim == 2)
  {
    vtkm::cont::CellSetStructured<2> cellSet;
    ds.GetCellSet(0).CopyTo(cellSet);
    vtkm::IdComponent shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_QUAD, "Wrong element type");
  }
  else if (dim == 3)
  {
    vtkm::cont::CellSetStructured<3> cellSet;
    ds.GetCellSet(0).CopyTo(cellSet);
    vtkm::IdComponent shape = cellSet.GetCellShape();
    VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_HEXAHEDRON, "Wrong element type");
  }
}

template <typename T>
vtkm::Range FillMethod(vtkm::IdComponent method,
                       vtkm::Id dimensionSize,
                       T &origin, T &spacing)
{
  switch (method)
  {
    case 0:
      origin = 0;
      spacing = 1;
      break;
    case 1:
      origin = 0;
      spacing = static_cast<T>(1.0/static_cast<double>(dimensionSize));
      break;
    case 2:
      origin = 0;
      spacing = 2;
      break;
    case 3:
      origin = static_cast<T>(-(dimensionSize-1));
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

  return vtkm::Range(origin, origin + static_cast<T>(dimensionSize-1)*spacing);
}

template <typename T>
void
UniformTests()
{
  const vtkm::Id NUM_TRIALS = 10;
  const vtkm::Id MAX_DIM_SIZE = 20;
  const vtkm::Id NUM_FILL_METHODS = 5;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;

  std::uniform_int_distribution<vtkm::Id> randomDim(2, MAX_DIM_SIZE);
  std::uniform_int_distribution<vtkm::IdComponent> randomFill(0, NUM_FILL_METHODS-1);

  for (vtkm::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    std::cout << "Trial " << trial << std::endl;

    vtkm::Id3 dimensions(randomDim(g_RandomGenerator),
                         randomDim(g_RandomGenerator),
                         randomDim(g_RandomGenerator));
    std::cout << "Dimensions: " << dimensions << std::endl;

    vtkm::IdComponent fillMethodX = randomFill(g_RandomGenerator);
    vtkm::IdComponent fillMethodY = randomFill(g_RandomGenerator);
    vtkm::IdComponent fillMethodZ = randomFill(g_RandomGenerator);
    std::cout << "Fill methods: ["
              << fillMethodX << ","
              << fillMethodY << ","
              << fillMethodZ << "]" << std::endl;

    vtkm::Vec<T,3> origin;
    vtkm::Vec<T,3> spacing;
    vtkm::Bounds bounds;
    bounds.X = FillMethod(fillMethodX,
                          dimensions[0],
                          origin[0],
                          spacing[0]);
    bounds.Y = FillMethod(fillMethodY,
                          dimensions[1],
                          origin[1],
                          spacing[1]);
    bounds.Z = FillMethod(fillMethodZ,
                          dimensions[2],
                          origin[2],
                          spacing[2]);

    std::cout << "3D case" << std::endl;
    vtkm::Id numPoints = dimensions[0]*dimensions[1]*dimensions[2];
    vtkm::Id numCells = (dimensions[0]-1)*(dimensions[1]-1)*(dimensions[2]-1);
    std::vector<T> varP3D(static_cast<unsigned long>(numPoints));
    for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
    {
      varP3D[i] = static_cast<T>(i*1.1f);
    }
    std::vector<T> varC3D(static_cast<unsigned long>(numCells));
    for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
    {
      varC3D[i] = static_cast<T>(i*1.1f);
    }
    dataSet = dataSetBuilder.Create(dimensions, origin, spacing);
    dsf.AddPointField(dataSet, "pointvar", varP3D);
    dsf.AddCellField(dataSet, "cellvar", varC3D);
    ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);

    std::cout << "2D case" << std::endl;
    numPoints = dimensions[0]*dimensions[1];
    numCells = (dimensions[0]-1)*(dimensions[1]-1);
    bounds.Z = vtkm::Range(0, 0);
    std::vector<T> varP2D(static_cast<unsigned long>(numPoints));
    for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
    {
      varP2D[i] = static_cast<T>(i*1.1f);
    }
    std::vector<T> varC2D(static_cast<unsigned long>(numCells));
    for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
    {
      varC2D[i] = static_cast<T>(i*1.1f);
    }
    dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                    vtkm::Vec<T,2>(origin[0], origin[1]),
                                    vtkm::Vec<T,2>(spacing[0], spacing[1]));
    dsf.AddPointField(dataSet, "pointvar", varP2D);
    dsf.AddCellField(dataSet, "cellvar", varC2D);
    ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);

    std::cout << "1D case" <<std::endl;
    numPoints = dimensions[0];
    numCells = dimensions[0]-1;
    bounds.Y = vtkm::Range(0, 0);
    bounds.Z = vtkm::Range(0, 0);
    std::vector<T> varP1D(static_cast<unsigned long>(numPoints));
    for (unsigned long i = 0; i < static_cast<unsigned long>(numPoints); i++)
    {
      varP1D[i] = static_cast<T>(i*1.1f);
    }
    std::vector<T> varC1D(static_cast<unsigned long>(numCells));
    for (unsigned long i = 0; i < static_cast<unsigned long>(numCells); i++)
    {
      varC1D[i] = static_cast<T>(i*1.1f);
    }
    dataSet = dataSetBuilder.Create(dimensions[0], origin[0], spacing[0]);
    dsf.AddPointField(dataSet, "pointvar", varP1D);
    dsf.AddCellField(dataSet, "cellvar", varC1D);
    ValidateDataSet(dataSet, 1, numPoints, numCells, bounds);
  }
}

void
TestDataSetBuilderUniform()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(time(NULL));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  UniformTests<vtkm::Float32>();
  std::cout << "======== Float64 ==========================" << std::endl;
  UniformTests<vtkm::Float64>();
}

} // namespace DataSetBuilderUniformNamespace

int UnitTestDataSetBuilderUniform(int, char *[])
{
    using namespace DataSetBuilderUniformNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderUniform);
}
