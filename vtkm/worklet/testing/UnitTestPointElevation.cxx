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

#include <vtkm/worklet/PointElevation.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

vtkm::cont::DataSet MakePointElevationTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Float32> xVals, yVals, zVals;
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
      xVals.push_back(x);
      yVals.push_back(y);
      zVals.push_back(z);
    }
  }

  vtkm::Id numVerts = dim * dim;
  vtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddField(vtkm::cont::Field("x", 1, vtkm::cont::Field::ASSOC_POINTS,
      &xVals[0], numVerts));
  dataSet.AddField(vtkm::cont::Field("y", 1, vtkm::cont::Field::ASSOC_POINTS,
      &yVals[0], numVerts));
  dataSet.AddField(vtkm::cont::Field("z", 1, vtkm::cont::Field::ASSOC_POINTS,
      &zVals[0], numVerts));
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("x","y","z"));

  vtkm::cont::CellSetExplicit<> cellSet(numVerts, "cells", 3);
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (vtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (vtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(vtkm::VTKM_QUAD,
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

void TestPointElevation()
{
  std::cout << "Testing PointElevation Worklet" << std::endl;

  vtkm::cont::DataSet dataSet = MakePointElevationTestDataSet();

  dataSet.AddField(vtkm::cont::Field("elevation", 1, vtkm::cont::Field::ASSOC_POINTS,
                                vtkm::Float32()));

  vtkm::worklet::PointElevation pointElevationWorklet;
  pointElevationWorklet.SetLowPoint(vtkm::make_Vec<vtkm::Float64>(0.0, 0.0, 0.0));
  pointElevationWorklet.SetHighPoint(vtkm::make_Vec<vtkm::Float64>(0.0, 1.0, 0.0));
  pointElevationWorklet.SetRange(0.0, 2.0);

  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointElevation>
      dispatcher(pointElevationWorklet);
  dispatcher.Invoke(dataSet.GetField("x").GetData(),
                    dataSet.GetField("y").GetData(),
                    dataSet.GetField("z").GetData(),
                    dataSet.GetField("elevation").GetData());

  vtkm::cont::ArrayHandle<vtkm::Float32> yVals =
      dataSet.GetField("y").GetData().CastToArrayHandle(vtkm::Float32(),
          VTKM_DEFAULT_STORAGE_TAG());
  vtkm::cont::ArrayHandle<vtkm::Float32> result =
      dataSet.GetField("elevation").GetData().CastToArrayHandle(vtkm::Float32(),
          VTKM_DEFAULT_STORAGE_TAG());

  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(test_equal(yVals.GetPortalConstControl().Get(i) * 2.0,
                                result.GetPortalConstControl().Get(i)),
       "Wrong result for PointElevation worklet");
  }
}

int UnitTestPointElevation(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestPointElevation);
}
