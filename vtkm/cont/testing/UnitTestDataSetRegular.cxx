//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

void TestDataSet_Regular()
{
    std::cout << std::endl;
    std::cout << "--TestDataSet_Regular--" << std::endl << std::endl;
    vtkm::cont::DataSet ds;

    ds.x_idx = 0;
    ds.y_idx = 1;
    ds.z_idx = 2;

    const int nVerts = 18;
    vtkm::Float32 xVals[nVerts] = {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2};
    vtkm::Float32 yVals[nVerts] = {0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1};
    vtkm::Float32 zVals[nVerts] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2};
    vtkm::Float32 vars[nVerts] = {10.1, 20.1, 30.1, 40.1, 50.2, 60.2, 70.2, 80.2, 90.3, 100.3, 110.3, 120.3, 130.4, 140.4, 150.4, 160.4, 170.5, 180.5};

    ds.AddFieldViaCopy(xVals, nVerts);
    ds.AddFieldViaCopy(yVals, nVerts);
    ds.AddFieldViaCopy(zVals, nVerts);

    //Set node scalar
    ds.AddFieldViaCopy(vars, nVerts);

    //Set cell scalar
    vtkm::Float32 cellvar[4] = {100.1, 100.2, 100.3, 100.4};
    ds.AddFieldViaCopy(cellvar, 4);

    vtkm::cont::CellSetStructured *cs = new vtkm::cont::CellSetStructured("cells");
    ds.AddCellSet(cs);

    //Set regular structure
    cs->structure.SetNodeDimension(3,2,3);

    //Run a worklet to populate a cell centered field.
    vtkm::Float32 cellVals[4] = {-1.1, -1.2, -1.3, -1.4};
    ds.AddFieldViaCopy(cellVals, 4);

    VTKM_TEST_ASSERT(test_equal(ds.GetNumberOfCellSets(), 1),
                     "Incorrect number of cell sets");

    VTKM_TEST_ASSERT(test_equal(ds.GetNumberOfFields(), 6),
                     "Incorrect number of fields");

    //cleanup memory now
    delete cs;
}

int UnitTestDataSetRegular(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestDataSet_Regular);
}