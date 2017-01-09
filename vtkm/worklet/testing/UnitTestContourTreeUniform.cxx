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

#include <vtkm/worklet/ContourTreeUniform.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>

//
// Test 2D regular dataset
//
vtkm::cont::DataSet MakeContourTreeMesh2DTestDataSet()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(5,5);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  const vtkm::Id nVerts = 25;
  vtkm::Float32 var[nVerts] = {
               100.0f, 78.0f, 49.0f, 17.0f,  1.0f,
                94.0f, 71.0f, 47.0f, 33.0f,  6.0f,
                52.0f, 44.0f, 50.0f, 45.0f, 48.0f,
                 8.0f, 12.0f, 46.0f, 91.0f, 43.0f,
                 0.0f,  5.0f, 51.0f, 76.0f, 83.0f};

  dsf.AddPointField(dataSet, "values", var, nVerts);

  return dataSet;
}

//
// Create a uniform 2D structured cell set as input with values for contours
//
void TestContourTree_Mesh2D_DEM_Triangulation()
{
  std::cout << "Testing ContourTree_Mesh2D Filter" << std::endl;

  // Create the input uniform cell set with values to contour
  vtkm::cont::DataSet dataSet = MakeContourTreeMesh2DTestDataSet();

  vtkm::cont::CellSetStructured<2> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::Id2 pointDimensions = cellSet.GetPointDimensions();
  vtkm::Id nRows = pointDimensions[0];
  vtkm::Id nCols = pointDimensions[1];

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("values").GetData().CopyTo(fieldArray);

  // Output saddle peak pairs array
  vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;

  // Create the worklet and run it
  vtkm::worklet::ContourTreeMesh2D contourTreeMesh2D;

  contourTreeMesh2D.Run(fieldArray,
                        nRows,
                        nCols,
                        saddlePeak,
                        DeviceAdapter());

  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 7), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(0), vtkm::make_Pair( 0, 12)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(1), vtkm::make_Pair( 4, 13)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(2), vtkm::make_Pair(12, 13)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(3), vtkm::make_Pair(12, 18)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(4), vtkm::make_Pair(12, 20)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(5), vtkm::make_Pair(13, 14)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(6), vtkm::make_Pair(13, 19)), "Wrong result for ContourTree filter");
}

//
// Test 3D regular dataset
//
vtkm::cont::DataSet MakeContourTreeMesh3DTestDataSet()
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id3 dimensions(5,5,5);
  vtkm::cont::DataSet dataSet = dsb.Create(dimensions);

  vtkm::cont::DataSetFieldAdd dsf;
  const vtkm::Id nVerts = 125;
  vtkm::Float32 var[nVerts] = {
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f, 99.0f, 90.0f, 85.0f,  0.0f,
                 0.0f, 95.0f, 80.0f, 95.0f,  0.0f,
                 0.0f, 85.0f, 90.0f, 99.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f, 75.0f, 50.0f, 65.0f,  0.0f,
                 0.0f, 55.0f, 15.0f, 45.0f,  0.0f,
                 0.0f, 60.0f, 40.0f, 70.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f, 97.0f, 87.0f, 82.0f,  0.0f,
                 0.0f, 92.0f, 77.0f, 92.0f,  0.0f,
                 0.0f, 82.0f, 87.0f, 97.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,

                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f,
                 0.0f,  0.0f,  0.0f,  0.0f,  0.0f};

  dsf.AddPointField(dataSet, "values", var, nVerts);

  return dataSet;
}

//
// Create a uniform 3D structured cell set as input with values for contours
//
void TestContourTree_Mesh3D_DEM_Triangulation()
{
  std::cout << "Testing ContourTree_Mesh3D Filter" << std::endl;

  // Create the input uniform cell set with values to contour
  vtkm::cont::DataSet dataSet = MakeContourTreeMesh3DTestDataSet();

  vtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().CopyTo(cellSet);

  vtkm::Id3 pointDimensions = cellSet.GetPointDimensions();
  vtkm::Id nRows = pointDimensions[0];
  vtkm::Id nCols = pointDimensions[1];
  vtkm::Id nSlices = pointDimensions[2];

  vtkm::cont::ArrayHandle<vtkm::Float32> fieldArray;
  dataSet.GetField("values").GetData().CopyTo(fieldArray);

  // Output saddle peak pairs array
  vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;

  // Create the worklet and run it
  vtkm::worklet::ContourTreeMesh3D contourTreeMesh3D;

  contourTreeMesh3D.Run(fieldArray,
                        nRows,
                        nCols,
                        nSlices,
                        saddlePeak,
                        DeviceAdapter());

  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 9), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(0), vtkm::make_Pair( 0, 67)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(1), vtkm::make_Pair(31, 42)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(2), vtkm::make_Pair(42, 43)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(3), vtkm::make_Pair(42, 56)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(4), vtkm::make_Pair(56, 67)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(5), vtkm::make_Pair(56, 92)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(6), vtkm::make_Pair(62, 67)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(7), vtkm::make_Pair(81, 92)), "Wrong result for ContourTree filter");
  VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(8), vtkm::make_Pair(92, 93)), "Wrong result for ContourTree filter");
}

void TestContourTreeUniform()
{
  TestContourTree_Mesh2D_DEM_Triangulation();
  TestContourTree_Mesh3D_DEM_Triangulation();
}

int UnitTestContourTreeUniform(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniform);
}
