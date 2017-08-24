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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/VertexClustering.h>

using vtkm::cont::testing::MakeTestDataSet;

namespace
{
}

void TestVertexClustering()
{
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet dataSet = maker.Make3DExplicitDataSetCowNose();

  vtkm::filter::VertexClustering clustering;
  vtkm::filter::Result result;

  clustering.SetNumberOfDivisions(vtkm::Id3(3, 3, 3));
  result = clustering.Execute(dataSet);

  VTKM_TEST_ASSERT(result.IsValid(), "results should be valid");

  vtkm::cont::DataSet output = result.GetDataSet();
  VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Number of output coordinate systems mismatch");

  // test
  const vtkm::Id output_points = 6;
  vtkm::Float64 output_point[output_points][3] = {
    { 0.0174716003, 0.0501927994, 0.0930275023 }, { 0.0320714004, 0.14704667, 0.0952706337 },
    { 0.0268670674, 0.246195346, 0.119720004 },   { 0.00215422804, 0.0340906903, 0.180881709 },
    { 0.0108188, 0.152774006, 0.167914003 },      { 0.0202241503, 0.225427493, 0.140208006 }
  };

  typedef vtkm::Vec<vtkm::Float64, 3> PointType;
  vtkm::cont::ArrayHandle<PointType> pointArray;
  output.GetCoordinateSystem(0).GetData().CopyTo(pointArray);
  VTKM_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points,
                   "Number of output points mismatch");
  for (vtkm::Id i = 0; i < pointArray.GetNumberOfValues(); ++i)
  {
    const PointType& p1 = pointArray.GetPortalConstControl().Get(i);
    PointType p2 = vtkm::make_Vec(output_point[i][0], output_point[i][1], output_point[i][2]);
    std::cout << "point: " << p1 << " " << p2 << std::endl;
    VTKM_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
  }
}

int UnitTestVertexClusteringFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering);
}
