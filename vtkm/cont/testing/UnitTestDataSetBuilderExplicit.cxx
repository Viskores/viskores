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

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/ExplicitTestData.h>

#include <vector>

namespace DataSetBuilderExplicitNamespace {

typedef vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DFA;
typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

template <typename T>
void ComputeBounds(std::size_t numPoints, const T *coords,
		   vtkm::Float64 *bounds)
{
    bounds[0] = bounds[1] = coords[0*3 +0];
    bounds[2] = bounds[3] = coords[0*3 +1];
    bounds[4] = bounds[5] = coords[0*3 +2];

    for (std::size_t i = 0; i < numPoints; i++)
    {
	bounds[0] = std::min(bounds[0], static_cast<vtkm::Float64>(coords[i*3+0]));
	bounds[1] = std::max(bounds[1], static_cast<vtkm::Float64>(coords[i*3+0]));
	bounds[2] = std::min(bounds[2], static_cast<vtkm::Float64>(coords[i*3+1]));
	bounds[3] = std::max(bounds[3], static_cast<vtkm::Float64>(coords[i*3+1]));
	bounds[4] = std::min(bounds[4], static_cast<vtkm::Float64>(coords[i*3+2]));
	bounds[5] = std::max(bounds[5], static_cast<vtkm::Float64>(coords[i*3+2]));
    }
}

void ValidateDataSet(const vtkm::cont::DataSet &ds,
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
    VTKM_TEST_ASSERT(bounds[0]==res[0] && bounds[1]==res[1] &&
                     bounds[2]==res[2] && bounds[3]==res[3] &&
                     bounds[4]==res[4] && bounds[5]==res[5],
                     "Bounds of coordinates do not match");
}
                         
template <typename T>
std::vector<T>
createVec(std::size_t n, const T *data)
{
    std::vector<T> vec(n);
    for (std::size_t i = 0; i < n; i++)
	vec[i] = data[i];
    return vec;
}

template <typename T>
vtkm::cont::ArrayHandle<T>
createAH(std::size_t n, const T *data)
{
    vtkm::cont::ArrayHandle<T> arr;
    DFA::Copy(vtkm::cont::make_ArrayHandle(data, static_cast<vtkm::Id>(n)), arr);
    return arr;
}

template <typename T>
vtkm::cont::DataSet
CreateDataSetArr(bool useSeparatedCoords,
		 std::size_t numPoints, const T *coords,
		 std::size_t numCells, std::size_t numConn,
		 const vtkm::Id *conn,
		 const vtkm::IdComponent *indices,
		 const vtkm::UInt8 *shape)
{
    vtkm::cont::DataSetBuilderExplicit dsb;
    if (useSeparatedCoords)
    {
	std::vector<T> xvals(numPoints), yvals(numPoints), zvals(numPoints);
	for (std::size_t i = 0; i < numPoints; i++)
	{
	    xvals[i] = coords[i*3 + 0];
	    yvals[i] = coords[i*3 + 1];
	    zvals[i] = coords[i*3 + 2];
	}
	vtkm::cont::ArrayHandle<T> X,Y,Z;
	DFA::Copy(vtkm::cont::make_ArrayHandle(xvals), X);
	DFA::Copy(vtkm::cont::make_ArrayHandle(yvals), Y);
	DFA::Copy(vtkm::cont::make_ArrayHandle(zvals), Z);
	return dsb.Create(X,Y,Z,
			  createAH(numCells, shape),
			  createAH(numCells, indices),
			  createAH(numConn, conn));
    }
    else
    {
	std::vector<vtkm::Vec<T,3> > tmp(numPoints);
	for (std::size_t i = 0; i < numPoints; i++)
	{
	    tmp[i][0] = coords[i*3 + 0];
	    tmp[i][1] = coords[i*3 + 1];
	    tmp[i][2] = coords[i*3 + 2];
	}

	vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > pts;
	DFA::Copy(vtkm::cont::make_ArrayHandle(tmp), pts);
	return dsb.Create(pts,
			  createAH(numCells, shape),
			  createAH(numCells, indices),
			  createAH(numConn, conn));
    }
}

template <typename T>
vtkm::cont::DataSet
CreateDataSetVec(bool useSeparatedCoords,
		 std::size_t numPoints, const T *coords,
		 std::size_t numCells, std::size_t numConn,
		 const vtkm::Id *conn,
		 const vtkm::IdComponent *indices,
		 const vtkm::UInt8 *shape)
{
    vtkm::cont::DataSetBuilderExplicit dsb;

    if (useSeparatedCoords)
    {
	std::vector<T> X(numPoints), Y(numPoints), Z(numPoints);
	for (std::size_t i = 0; i < numPoints; i++)
	{
		X[i] = coords[i*3 + 0];
		Y[i] = coords[i*3 + 1];
		Z[i] = coords[i*3 + 2];
	}
	return dsb.Create(X,Y,Z,
			  createVec(numCells, shape),
			  createVec(numCells, indices),
			  createVec(numConn, conn));
    }
    else
    {
	std::vector<vtkm::Vec<T,3> > pts(numPoints);
	for (std::size_t i = 0; i < numPoints; i++)
        {
	    pts[i][0] = coords[i*3 + 0];
	    pts[i][1] = coords[i*3 + 1];
	    pts[i][2] = coords[i*3 + 2];
	}
	return dsb.Create(pts,
			  createVec(numCells, shape),
			  createVec(numCells, indices),
			  createVec(numConn, conn));
    }
}

#define TEST_DATA(num) \
    vtkm::cont::testing::ExplicitData##num::numPoints, \
    vtkm::cont::testing::ExplicitData##num::coords, \
    vtkm::cont::testing::ExplicitData##num::numCells, \
    vtkm::cont::testing::ExplicitData##num::numConn, \
    vtkm::cont::testing::ExplicitData##num::conn, \
    vtkm::cont::testing::ExplicitData##num::numIndices, \
    vtkm::cont::testing::ExplicitData##num::shapes
#define TEST_NUMS(num) \
    vtkm::cont::testing::ExplicitData##num::numPoints, \
    vtkm::cont::testing::ExplicitData##num::numCells
#define TEST_BOUNDS(num) \
    vtkm::cont::testing::ExplicitData##num::numPoints, \
    vtkm::cont::testing::ExplicitData##num::coords

void
TestDataSetBuilderExplicit()
{
    vtkm::cont::DataSetBuilderExplicit dsb;
    vtkm::cont::DataSet ds;
    vtkm::Float64 bounds[6];

    //Iterate over organization of coordinates.
    for (int i = 0; i < 2; i++)
    {
	//Test ExplicitData0
	ComputeBounds(TEST_BOUNDS(0), bounds);
	ds = CreateDataSetArr(i==0,TEST_DATA(0));
	ValidateDataSet(ds, TEST_NUMS(0), bounds);
	ds = CreateDataSetVec(i==0, TEST_DATA(0));
	ValidateDataSet(ds, TEST_NUMS(0), bounds);

	//Test ExplicitData1
	ComputeBounds(TEST_BOUNDS(1), bounds);
	ds = CreateDataSetArr(i==0,TEST_DATA(1));
	ValidateDataSet(ds, TEST_NUMS(1), bounds);
	ds = CreateDataSetVec(i==0, TEST_DATA(1));
	ValidateDataSet(ds, TEST_NUMS(1), bounds);

	//Test ExplicitData2
	ComputeBounds(TEST_BOUNDS(2), bounds);
	ds = CreateDataSetArr(i==0,TEST_DATA(2));
	ValidateDataSet(ds, TEST_NUMS(2), bounds);
	ds = CreateDataSetVec(i==0, TEST_DATA(2));
	ValidateDataSet(ds, TEST_NUMS(2), bounds);
    }
}

} // namespace DataSetBuilderExplicitNamespace

int UnitTestDataSetBuilderExplicit(int, char *[])
{
    using namespace DataSetBuilderExplicitNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderExplicit);
}
