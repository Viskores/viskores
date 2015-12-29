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

void ValidateDataSet(const vtkm::cont::DataSet &ds,
		     int dim,
		     int numPoints, int numCells)
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
			 

void
TestDataSetBuilderRegular()
{
    vtkm::cont::DataSetBuilderRegular dsb;
    vtkm::cont::DataSet ds;

    int nx = 20, ny = 20, nz = 20;
    //2D cases.
    for (int i = 2; i < nx; i++)
	for (int j = 2; j < ny; j++)
	{
	    vtkm::Id2 dims(i,j);
	    ds = dsb.Create(dims);
	    ValidateDataSet(ds, 2, i*j, (i-1)*(j-1));
	}

    //3D cases.
    for (int i = 2; i < nx; i++)
	for (int j = 2; j < ny; j++)
	    for (int k = 2; k < nz; k++)
	{
	    vtkm::Id3 dims(i,j,k);
	    ds = dsb.Create(dims);
	    ValidateDataSet(ds, 3, i*j*k, (i-1)*(j-1)*(k-1));
	}

}

} // namespace ArrayHandleCartesianProductNamespace

int UnitTestDataSetBuilderRegular(int, char *[])
{
    using namespace DataSetBuilderRegularNamespace;
    return vtkm::cont::testing::Testing::Run(TestDataSetBuilderRegular);
}
