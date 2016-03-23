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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/filter/PointElevation.h>
#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

vtkm::cont::DataSet MakePointElevationTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec<vtkm::Float32,3> > coordinates;
  const vtkm::Id dim = 5;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    vtkm::Float32 z = static_cast<vtkm::Float32>(j) /
                      static_cast<vtkm::Float32>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      vtkm::Float32 x = static_cast<vtkm::Float32>(i) /
                        static_cast<vtkm::Float32>(dim - 1);
      vtkm::Float32 y = (x*x + z*z)/2.0f;
      coordinates.push_back(vtkm::make_Vec(x,y,z));
    }
  }

  vtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem("coordinates", coordinates));

  vtkm::cont::CellSetExplicit<> cellSet(vtkm::Id(coordinates.size()), "cells", 3);
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (vtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (vtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(vtkm::CELL_SHAPE_QUAD,
                      4,
                      vtkm::make_Vec<vtkm::Id>(j * dim + i,
                                               j * dim + i + 1,
                                               (j + 1) * dim + i + 1,
                                               (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells();

  dataSet.AddCellSet(cellSet);
  return dataSet;
}

}

void TestPointElevationNoPolicy()
{
  std::cout << "Testing PointElevation Filter With No Policy" << std::endl;

  vtkm::cont::DataSet inputData = MakePointElevationTestDataSet();

  vtkm::filter::PointElevation filter;
  filter.SetLowPoint( 0.0, 0.0, 0.0 );
  filter.SetHighPoint( 0.0, 1.0, 0.0 );
  filter.SetRange( 0.0, 2.0 );

  filter.SetOutputFieldName("elevation");
  vtkm::filter::FieldResult result;
  result = filter.Execute(inputData,
                          inputData.GetCoordinateSystem());

  //verify the result
  VTKM_TEST_ASSERT( result.IsValid(), "result should be valid" );

  vtkm::cont::ArrayHandle<vtkm::Float64> resultArrayHandle;
  const bool valid = result.FieldAs(resultArrayHandle);
  if(valid)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3> > coordinates;
    inputData.GetCoordinateSystem().GetData().CopyTo(coordinates);

    for (vtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(coordinates.GetPortalConstControl().Get(i)[1] * 2.0,
                                  resultArrayHandle.GetPortalConstControl().Get(i)),
         "Wrong result for PointElevation worklet");
    }
  }
}

void TestPointElevationWithPolicy()
{

  //simple test
  std::cout << "Testing PointElevation Filter With Explicit Policy" << std::endl;

  vtkm::cont::DataSet inputData = MakePointElevationTestDataSet();

  vtkm::filter::PointElevation filter;
  filter.SetLowPoint( 0.0, 0.0, 0.0 );
  filter.SetHighPoint( 0.0, 1.0, 0.0 );
  filter.SetRange( 0.0, 2.0 );

  filter.SetOutputFieldName("elevation");
  vtkm::filter::FieldResult result;

  vtkm::filter::DefaultPolicy p;
  result = filter.Execute(inputData,
                          inputData.GetCoordinateSystem(),
                          p);

  //verify the result
  VTKM_TEST_ASSERT( result.IsValid(), "result should be valid" );

  vtkm::cont::ArrayHandle<vtkm::Float64> resultArrayHandle;
  const bool valid = result.FieldAs(resultArrayHandle, p);
  if(valid)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,3> > coordinates;
    inputData.GetCoordinateSystem().GetData().CopyTo(coordinates);

    for (vtkm::Id i = 0; i < resultArrayHandle.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(coordinates.GetPortalConstControl().Get(i)[1] * 2.0,
                                  resultArrayHandle.GetPortalConstControl().Get(i)),
         "Wrong result for PointElevation worklet");
    }
  }

}

void TestPointElevation()
{
  TestPointElevationNoPolicy();
  TestPointElevationWithPolicy();
}

int UnitTestPointElevationFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestPointElevation);
}
