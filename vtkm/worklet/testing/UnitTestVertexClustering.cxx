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

#include <iostream>
#include <algorithm>

#include <vtkm/worklet/PointElevation.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/worklet/VertexClustering.h>
namespace{

void TestVertexClustering()
{
  double bounds[6];
  const int divisions = 3;
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet ds = maker.Make3DExplicitDataSetCowNose(bounds);

  // run
  vtkm::cont::DataSet ds_out = vtkm::worklet::VertexClustering<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().run(ds, bounds, divisions);

  // test
  const int output_pointIds = 9;
  int output_pointId[output_pointIds] = {1,2,5, 1,3,0, 1,5,4};
  const int output_points = 6;
  double output_point[output_points][3] = {{0.0174716003,0.0501927994,0.0930275023}, {0.0320714004,0.14704667,0.0952706337}, {0.0268670674,0.246195346,0.119720004}, {0.00215422804,0.0340906903,0.180881709}, {0.0108188,0.152774006,0.167914003}, {0.0202241503,0.225427493,0.140208006}};

  vtkm::Id i;
  VTKM_TEST_ASSERT(ds_out.GetNumberOfFields() == 1, "Number of output fields mismatch");
  typedef vtkm::Vec<vtkm::Float32, 3> PointType;
  typedef vtkm::cont::ArrayHandle<PointType > PointArray;
  PointArray pointArray = ds_out.GetField(0).GetData().CastToArrayHandle<PointArray::ValueType, PointArray::StorageTag>();
  VTKM_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points, "Number of output points mismatch" );
  for (i = 0; i < pointArray.GetNumberOfValues(); ++i)
    {
      const PointType &p1 = pointArray.GetPortalConstControl().Get(i);
      PointType p2 = vtkm::make_Vec<vtkm::Float32>((vtkm::Float32)output_point[i][0], (vtkm::Float32)output_point[i][1], (vtkm::Float32)output_point[i][2]) ;
      std::cout << "point: " << p1 << " " << p2 << std::endl;
      //VTKM_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
    }

  VTKM_TEST_ASSERT(ds_out.GetNumberOfCellSets() == 1, "Number of output cellsets mismatch");
  vtkm::cont::CellSetExplicit<> *cellset = dynamic_cast<vtkm::cont::CellSetExplicit<> *>(ds_out.GetCellSet(0).get());
  VTKM_TEST_ASSERT(cellset, "CellSet Cast fail");
  vtkm::cont::ExplicitConnectivity<> &conn = cellset->GetNodeToCellConnectivity();
  VTKM_TEST_ASSERT(conn.GetConnectivityArray().GetNumberOfValues() == output_pointIds, "Number of connectivity array elements mismatch");
  for (i=0; i<conn.GetConnectivityArray().GetNumberOfValues(); i++)
    {
      vtkm::Id id1 = conn.GetConnectivityArray().GetPortalConstControl().Get(i) ;
      vtkm::Id id2 = output_pointId[i] ;
      std::cout << "pointid: " << id1 << " " << id2 << std::endl;
      //VTKM_TEST_ASSERT( id1 == id2, "Connectivity Array mismatch" )  ;
    }

} // TestVertexClustering

} // namespace

int UnitTestVertexClustering(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering);
}

