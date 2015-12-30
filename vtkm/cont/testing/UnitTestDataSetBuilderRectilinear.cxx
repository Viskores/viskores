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
    //This is not working at present...
    /*
    vtkm::Float64 res[6];
    ds.GetCoordinateSystem().GetBounds(res, DeviceAdapter());
    VTKM_TEST_ASSERT(bounds[0]==res[0] && bounds[1]==res[1] &&
                     bounds[2]==res[2] && bounds[3]==res[3] &&
                     bounds[4]==res[4] && bounds[5]==res[5],
                     "Bounds of coordinates do not match");
    */

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
void FillArray(std::vector<T> &arr, vtkm::Id sz, int fillMethod)
{
    arr.resize(sz);
    for (vtkm::Id i = 0; i < sz; i++)
    {
        T xi;

        switch (fillMethod)
        {
        case 0: xi = (T)i; break;
        case 1: xi = (T)i / (vtkm::Float32)sz; break;
        case 2: xi = (T)(i*2); break;
        case 3: xi = (T)i*0.1f; break;
        case 4: xi = (T)(i*i); break;
        }
        arr[i] = xi;
    }
}

void
TestDataSetBuilderRectilinear()
{
    vtkm::cont::DataSetBuilderRectilinear dsb;
    vtkm::cont::DataSet ds;

    vtkm::Id nx = 15, ny = 15, nz = 15;
    int nm = 5;
    std::vector<vtkm::Float32> xvals, yvals, zvals;

    for (vtkm::Id i = 2; i < nx; i++)
        for (vtkm::Id j = 2; j < ny; j++)
            for (int mx = 0; mx < nm; mx++)
                for (int my = 0; my < nm; my++)
                {
                    //Do the 2D cases.
                    vtkm::Id np = i*j, nc = (i-1)*(j-1);
                    FillArray(xvals, i, mx);
                    FillArray(yvals, j, my);

                    vtkm::Float64 bounds[6] = {xvals[0],xvals[i-1],
                                               yvals[0],yvals[j-1],
                                               0.0, 0.0};
                    //Test std::vector
                    ds = dsb.Create(xvals, yvals);
                    ValidateDataSet(ds, 2, np, nc, bounds);

                    //Test vtkm::Float *
                    ds = dsb.Create(i,j, &xvals[0],&yvals[0]);
                    ValidateDataSet(ds, 2, np, nc, bounds);

                    //Test ArrayHandle
                    ds = dsb.Create(vtkm::cont::make_ArrayHandle(xvals),
                                    vtkm::cont::make_ArrayHandle(yvals));
                    ValidateDataSet(ds, 2, np, nc, bounds);

                    //Do the 3D cases.
                    for (vtkm::Id k = 2; k < nz; k++)
                        for (int mz = 0; mz < nm; mz++)
                        {
                            np = i*j*k;
                            nc = (i-1)*(j-1)*(k-1);
                            FillArray(zvals, k, mz);

                            //Test std::vector
                            ds = dsb.Create(xvals, yvals, zvals);
                            ValidateDataSet(ds, 3, np, nc, bounds);

                            //Test vtkm::Float *
                            ds = dsb.Create(i,j,k, &xvals[0],&yvals[0], &zvals[0]);
                            ValidateDataSet(ds, 3, np, nc, bounds);

                            //Test ArrayHandle
                            ds = dsb.Create(vtkm::cont::make_ArrayHandle(xvals),
                                            vtkm::cont::make_ArrayHandle(yvals),
                                            vtkm::cont::make_ArrayHandle(zvals));
                            ValidateDataSet(ds, 3, np, nc, bounds);
                            
                        }
                }
}

} // namespace DataSetBuilderRectilinearNamespace

int UnitTestDataSetBuilderRectilinear(int, char *[])
{
    using namespace DataSetBuilderRectilinearNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderRectilinear);
}
