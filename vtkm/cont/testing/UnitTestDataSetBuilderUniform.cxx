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

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Assert.h>

#include <vtkm/cont/testing/Testing.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/static_assert.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <time.h>

#include <vector>

namespace DataSetBuilderUniformNamespace {

boost::mt19937 g_RandomGenerator;

typedef vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DFA;
typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

void ValidateDataSet(const vtkm::cont::DataSet &ds,
                     int dim,
                     vtkm::Id numPoints, vtkm::Id numCells,
                     vtkm::Float64 *bounds)
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
    VTKM_TEST_ASSERT(ds.GetCellSet().GetCellSet().GetNumberOfCells() == numCells,
                     "Wrong number of cells.");


    //Make sure bounds are correct.
    vtkm::Float64 res[6];
    ds.GetCoordinateSystem().GetBounds(res, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(bounds[0], res[0]) && test_equal(bounds[1], res[1]) &&
                     test_equal(bounds[2], res[2]) && test_equal(bounds[3], res[3]) &&
                     test_equal(bounds[4], res[4]) && test_equal(bounds[5], res[5]),
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
void FillMethod(vtkm::IdComponent method,
                vtkm::Id dimensionSize,
                T &origin,
                T &spacing,
                vtkm::Float64 &boundsMin,
                vtkm::Float64 &boundsMax)
{
  switch (method)
  {
    case 0:
      origin = 0;
      spacing = 1;
      break;
    case 1:
      origin = 0;
      spacing = static_cast<T>(1.0/dimensionSize);
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
  }

  boundsMin = static_cast<vtkm::Float64>(origin);
  boundsMax = static_cast<vtkm::Float64>(origin + (dimensionSize-1)*spacing);
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

    vtkm::Vec<T,3> origin;
    vtkm::Vec<T,3> spacing;
    vtkm::Float64 bounds[6];
    FillMethod(fillMethodX,
               dimensions[0],
               origin[0],
               spacing[0],
               bounds[0],
               bounds[1]);
    FillMethod(fillMethodY,
               dimensions[1],
               origin[1],
               spacing[1],
               bounds[2],
               bounds[3]);
    FillMethod(fillMethodZ,
               dimensions[2],
               origin[2],
               spacing[2],
               bounds[4],
               bounds[5]);

    std::cout << "3D case" << std::endl;
    vtkm::Id numPoints = dimensions[0]*dimensions[1]*dimensions[2];
    vtkm::Id numCells = (dimensions[0]-1)*(dimensions[1]-1)*(dimensions[2]-1);
    dataSet = dataSetBuilder.Create(dimensions, origin, spacing);
    ValidateDataSet(dataSet, 3, numPoints, numCells, bounds);

    std::cout << "2D case" << std::endl;
    numPoints = dimensions[0]*dimensions[1];
    numCells = (dimensions[0]-1)*(dimensions[1]-1);
    bounds[4] = bounds[5] = 0.0;
    dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                    vtkm::Vec<T,2>(origin[0], origin[1]),
                                    vtkm::Vec<T,2>(spacing[0], spacing[1]));
    ValidateDataSet(dataSet, 2, numPoints, numCells, bounds);
  }

#if 0
    vtkm::cont::DataSetBuilderUniform dsb;
    vtkm::cont::DataSet ds;

    vtkm::Id nx = 12, ny = 12, nz = 12;
    int nm = 5;
    vtkm::Float64 bounds[6];

    for (vtkm::Id i = 2; i < nx; i++)
        for (vtkm::Id j = 2; j < ny; j++)
            for (int mi = 0; mi < nm; mi++)
                for (int mj = 0; mj < nm; mj++)
                {
                    //2D cases
                    vtkm::Id np = i*j, nc = (i-1)*(j-1);

                    vtkm::Id2 dims2(i,j);
                    T oi, oj, si, sj;
                    FillMethod(mi, dims2[0], oi, si, bounds[0],bounds[1]);
                    FillMethod(mj, dims2[1], oj, sj, bounds[2],bounds[3]);
                    bounds[4] = bounds[5] = 0;
                    vtkm::Vec<T,2> o2(oi,oj), sp2(si,sj);

                    ds = dsb.Create(dims2, o2, sp2);
                    ValidateDataSet(ds, 2, np, nc, bounds);

                    //3D cases
                    for (vtkm::Id k = 2; k < nz; k++)
                        for (int mk = 0; mk < nm; mk++)
                        {
                            np = i*j*k;
                            nc = (i-1)*(j-1)*(k-1);

                            vtkm::Id3 dims3(i,j,k);
                            T ok, sk;
                            FillMethod(mk, dims3[2], ok, sk, bounds[4],bounds[5]);
                            vtkm::Vec<T,3> o3(oi,oj,ok), sp3(si,sj,sk);
                            ds = dsb.Create(dims3, o3, sp3);
                            ValidateDataSet(ds, 3, np, nc, bounds);
                        }
                }
#endif
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
