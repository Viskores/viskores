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
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/cont/testing/Testing.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <ctime>

#include <vector>

namespace DataSetBuilderRectilinearNamespace {

boost::mt19937 g_RandomGenerator;

typedef vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DFA;
typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

void ValidateDataSet(const vtkm::cont::DataSet &ds,
                     int dim,
                     vtkm::Id numPoints, vtkm::Id numCells,
                     const vtkm::Bounds &bounds)
{
    //Verify basics..
    VTKM_TEST_ASSERT(ds.GetNumberOfCellSets() == 1,
                     "Wrong number of cell sets.");
    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 0,
                     "Wrong number of fields.");
    VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                     "Wrong number of coordinate systems.");
    VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == numPoints,
                     "Wrong number of coordinates.");
    VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == numCells,
                     "Wrong number of cells.");

    //Make sure the bounds are correct.
    vtkm::Bounds res = ds.GetCoordinateSystem().GetBounds(DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(bounds, res),
                     "Bounds of coordinates do not match");
    if (dim == 2)
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
void FillArray(std::vector<T> &arr,
               vtkm::Id size,
               vtkm::IdComponent fillMethod)
{
  arr.resize(static_cast<std::size_t>(size));
  for (size_t i = 0; i < static_cast<std::size_t>(size); i++)
  {
    T xi = static_cast<T>(i);

    switch (fillMethod)
    {
      case 0: break;
      case 1: xi /= static_cast<vtkm::Float32>(size-1); break;
      case 2: xi *= 2; break;
      case 3: xi *= 0.1f; break;
      case 4: xi *= xi; break;
      default: VTKM_TEST_FAIL("Bad internal test state: invalid fill method.");
    }
    arr[i] = xi;
  }
}

template <typename T>
void
RectilinearTests()
{
  const vtkm::Id NUM_TRIALS = 10;
  const vtkm::Id MAX_DIM_SIZE = 20;
  const vtkm::Id NUM_FILL_METHODS = 5;

  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;
  vtkm::cont::DataSet dataSet;

  boost::random::uniform_int_distribution<vtkm::Id>
      randomDim(2, MAX_DIM_SIZE);
  boost::random::uniform_int_distribution<vtkm::IdComponent>
      randomFill(0, NUM_FILL_METHODS-1);

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

    std::vector<T> xCoordinates;
    std::vector<T> yCoordinates;
    std::vector<T> zCoordinates;
    FillArray(xCoordinates, dimensions[0], fillMethodX);
    FillArray(yCoordinates, dimensions[1], fillMethodY);
    FillArray(zCoordinates, dimensions[2], fillMethodZ);

    std::cout << "2D cases" << std::endl;
    vtkm::Id numPoints = dimensions[0]*dimensions[1];
    vtkm::Id numCells = (dimensions[0]-1)*(dimensions[1]-1);
    vtkm::Bounds bounds(xCoordinates.front(), xCoordinates.back(),
                        yCoordinates.front(), yCoordinates.back(),
                        0.0, 0.0);

    std::cout << "  Create with std::vector" << std::endl;
    dataSet = dataSetBuilder.Create(xCoordinates, yCoordinates);
    ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);

    std::cout << "  Create with C array" << std::endl;
    dataSet = dataSetBuilder.Create(dimensions[0],
                                    dimensions[1],
                                    &xCoordinates.front(),
                                    &yCoordinates.front());
    ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);

    std::cout << "  Create with ArrayHandle" << std::endl;
    dataSet = dataSetBuilder.Create(vtkm::cont::make_ArrayHandle(xCoordinates),
                                    vtkm::cont::make_ArrayHandle(yCoordinates));
    ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);

    std::cout << "3D cases" << std::endl;
    numPoints *= dimensions[2];
    numCells *= dimensions[2]-1;
    bounds.Z = vtkm::Range(zCoordinates.front(), zCoordinates.back());

    std::cout << "  Create with std::vector" << std::endl;
    dataSet = dataSetBuilder.Create(xCoordinates, yCoordinates, zCoordinates);
    ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);

    std::cout << "  Create with C array" << std::endl;
    dataSet = dataSetBuilder.Create(dimensions[0],
                                    dimensions[1],
                                    dimensions[2],
                                    &xCoordinates.front(),
                                    &yCoordinates.front(),
                                    &zCoordinates.front());
    ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);

    std::cout << "  Create with ArrayHandle" << std::endl;
    dataSet = dataSetBuilder.Create(vtkm::cont::make_ArrayHandle(xCoordinates),
                                    vtkm::cont::make_ArrayHandle(yCoordinates),
                                    vtkm::cont::make_ArrayHandle(zCoordinates));
    ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);
  }
}

void
TestDataSetBuilderRectilinear()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  RectilinearTests<vtkm::Float32>();
  std::cout << "======== Float64 ==========================" << std::endl;
  RectilinearTests<vtkm::Float64>();
}

} // namespace DataSetBuilderRectilinearNamespace

int UnitTestDataSetBuilderRectilinear(int, char *[])
{
    using namespace DataSetBuilderRectilinearNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderRectilinear);
}
