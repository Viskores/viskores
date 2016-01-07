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

#include <vtkm/cont/DataSetBuilderRegular.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Assert.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace DataSetBuilderRegularNamespace {

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
void FillMethod(int method, vtkm::Id n, T &o, T &s,
                vtkm::Float64 &b0, vtkm::Float64 &b1)
{
    switch (method)
    {
    case 0 : o = 0; s = 1; break;
    case 1 : o = 0; s = static_cast<T>(1.0/n); break;
    case 2 : o = 0; s = 2; break;
    case 3 : o = static_cast<T>(-(n-1)); s = 1; break;
    case 4 : o = static_cast<T>(2.780941); s = static_cast<T>(182.381901); break;
    }

    b0 = static_cast<vtkm::Float64>(o);
    b1 = static_cast<vtkm::Float64>(o + (n-1)*s);
}

template <typename T>
void
RegularTests()
{
    vtkm::cont::DataSetBuilderRegular dsb;
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
}

void
TestDataSetBuilderRegular()
{
    RegularTests<vtkm::Float32>();
    RegularTests<vtkm::Float64>();
}

} // namespace DataSetBuilderRegularNamespace

int UnitTestDataSetBuilderRegular(int, char *[])
{
    using namespace DataSetBuilderRegularNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderRegular);
}
