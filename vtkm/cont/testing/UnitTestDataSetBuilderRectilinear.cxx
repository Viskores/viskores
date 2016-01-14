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

#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Assert.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace DataSetBuilderRectilinearNamespace {

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

    //Make sure the bounds are correct.
    vtkm::Float64 res[6];
    ds.GetCoordinateSystem().GetBounds(res, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(bounds[0], res[0]) && test_equal(bounds[1], res[1]) &&
                     test_equal(bounds[2], res[2]) && test_equal(bounds[3], res[3]) &&
                     test_equal(bounds[4], res[4]) && test_equal(bounds[5], res[5]),
                     "Bounds of coordinates do not match");
    if (dim == 2)
    {
        typedef vtkm::cont::CellSetStructured<2> CellSetType;
        CellSetType cellSet = ds.GetCellSet(0).CastTo<CellSetType>();
        vtkm::IdComponent shape = cellSet.GetCellShape();
        VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_QUAD, "Wrong element type");
    }
    else if (dim == 3)
    {
        typedef vtkm::cont::CellSetStructured<3> CellSetType;
        CellSetType cellSet = ds.GetCellSet(0).CastTo<CellSetType>();
        vtkm::IdComponent shape = cellSet.GetCellShape();
        VTKM_TEST_ASSERT(shape == vtkm::CELL_SHAPE_HEXAHEDRON, "Wrong element type");
    }
}

template <typename T>
void FillArray(std::vector<T> &arr, std::size_t sz, int fillMethod)
{
    arr.resize(sz);
    for (size_t i = 0; i < sz; i++)
    {
        T xi;

        switch (fillMethod)
        {
        case 0: xi = static_cast<T>(i); break;
        case 1: xi = static_cast<T>(i) / static_cast<vtkm::Float32>(sz-1); break;
        case 2: xi = static_cast<T>(i*2); break;
        case 3: xi = static_cast<T>(i*0.1f); break;
        case 4: xi = static_cast<T>(i*i); break;
        }
        arr[i] = xi;
    }
}

template <typename T>
void
RectilinearTests()
{
  vtkm::cont::DataSetBuilderRectilinear dsb;
  vtkm::cont::DataSet ds;

  std::size_t nx = 15, ny = 15, nz = 15;
  int nm = 5;
  std::vector<T> xvals, yvals, zvals;

  for (std::size_t i = 2; i < nx; i++)
  {
    for (std::size_t j = 2; j < ny; j++)
    {
      for (int mx = 0; mx < nm; mx++)
      {
        for (int my = 0; my < nm; my++)
        {
          //Do the 2D cases.
          vtkm::Id np = static_cast<vtkm::Id>(i*j);
          vtkm::Id nc = static_cast<vtkm::Id>((i-1)*(j-1));
          FillArray(xvals, i, mx);
          FillArray(yvals, j, my);

          vtkm::Float64 bounds[6] = {xvals[0], xvals[i-1],
                                     yvals[0], yvals[j-1],
                                     0.0, 0.0};
          //Test std::vector
          ds = dsb.Create(xvals, yvals);
          ValidateDataSet(ds, 2, np, nc, bounds);

          //Test T *
          ds = dsb.Create(static_cast<vtkm::Id>(i),
                          static_cast<vtkm::Id>(j),
                          &xvals[0],
                          &yvals[0]);
          ValidateDataSet(ds, 2, np, nc, bounds);

          //Test ArrayHandle
          ds = dsb.Create(vtkm::cont::make_ArrayHandle(xvals),
                          vtkm::cont::make_ArrayHandle(yvals));
          ValidateDataSet(ds, 2, np, nc, bounds);

          //Do the 3D cases.
          for (std::size_t k = 2; k < nz; k++)
          {
            for (int mz = 0; mz < nm; mz++)
            {
              np = static_cast<vtkm::Id>(i*j*k);
              nc = static_cast<vtkm::Id>((i-1)*(j-1)*(k-1));
              FillArray(zvals, k, mz);
              bounds[4] = zvals[0];
              bounds[5] = zvals[k-1];

              //Test std::vector
              ds = dsb.Create(xvals, yvals, zvals);
              ValidateDataSet(ds, 3, np, nc, bounds);

              //Test T *
              ds = dsb.Create(static_cast<vtkm::Id>(i),
                              static_cast<vtkm::Id>(j),
                              static_cast<vtkm::Id>(k),
                              &xvals[0],
                              &yvals[0],
                              &zvals[0]);
              ValidateDataSet(ds, 3, np, nc, bounds);

              //Test ArrayHandle
              ds = dsb.Create(vtkm::cont::make_ArrayHandle(xvals),
                              vtkm::cont::make_ArrayHandle(yvals),
                              vtkm::cont::make_ArrayHandle(zvals));
              ValidateDataSet(ds, 3, np, nc, bounds);
            }
          }
        }
      }
    }
  }
}

void
TestDataSetBuilderRectilinear()
{
    RectilinearTests<vtkm::Float32>();
    RectilinearTests<vtkm::Float64>();
}

} // namespace DataSetBuilderRectilinearNamespace

int UnitTestDataSetBuilderRectilinear(int, char *[])
{
    using namespace DataSetBuilderRectilinearNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderRectilinear);
}
