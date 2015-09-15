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

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/VertexClustering.h>


void TestVertexClustering()
{
  vtkm::Float64 bounds[6];
  const vtkm::Id divisions = 3;
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet dataSet = maker.Make3DExplicitDataSetCowNose(bounds);

  // run
  vtkm::worklet::VertexClustering<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> clustering;
  vtkm::cont::DataSet outDataSet = clustering.Run(dataSet.GetCellSet(),
                                                  dataSet.GetCoordinateSystem(),
                                                  divisions);

  // test
  const vtkm::Id output_pointIds = 9;
  vtkm::Id output_pointId[output_pointIds] = {0,1,3, 1,5,4, 1,2,5};
  const vtkm::Id output_points = 6;
  vtkm::Float64 output_point[output_points][3] = {{0.0174716003,0.0501927994,0.0930275023}, {0.0320714004,0.14704667,0.0952706337}, {0.0268670674,0.246195346,0.119720004}, {0.00215422804,0.0340906903,0.180881709}, {0.0108188,0.152774006,0.167914003}, {0.0202241503,0.225427493,0.140208006}};

  VTKM_TEST_ASSERT(outDataSet.GetNumberOfCoordinateSystems() == 1,
                   "Number of output coordinate systems mismatch");
  typedef vtkm::Vec<vtkm::Float64, 3> PointType;
  typedef vtkm::cont::ArrayHandle<PointType > PointArray;
  PointArray pointArray =
      outDataSet.GetCoordinateSystem(0).GetData().
      CastToArrayHandle<PointArray::ValueType, PointArray::StorageTag>();
  VTKM_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points,
                   "Number of output points mismatch" );
  for (vtkm::Id i = 0; i < pointArray.GetNumberOfValues(); ++i)
    {
      const PointType &p1 = pointArray.GetPortalConstControl().Get(i);
      PointType p2 = vtkm::make_Vec(output_point[i][0],
                                    output_point[i][1],
                                    output_point[i][2]);
      std::cout << "point: " << p1 << " " << p2 << std::endl;
      VTKM_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
    }

  typedef vtkm::cont::CellSetExplicit<
      vtkm::cont::ArrayHandleConstant<vtkm::Id>::StorageTag,
      vtkm::cont::ArrayHandleConstant<vtkm::Id>::StorageTag,
      VTKM_DEFAULT_STORAGE_TAG> CellSetType;

  VTKM_TEST_ASSERT(outDataSet.GetNumberOfCellSets() == 1, "Number of output cellsets mismatch");
  CellSetType &cellSet = outDataSet.GetCellSet(0).CastTo<CellSetType>();
  VTKM_TEST_ASSERT(
        cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell()).GetNumberOfValues() == output_pointIds,
        "Number of connectivity array elements mismatch");
  for (vtkm::Id i=0; i<cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell()).GetNumberOfValues(); i++)
    {
      vtkm::Id id1 = cellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell()).GetPortalConstControl().Get(i) ;
      vtkm::Id id2 = output_pointId[i] ;
      std::cout << "pointid: " << id1 << " " << id2 << std::endl;
      //VTKM_TEST_ASSERT( id1 == id2, "Connectivity Array mismatch" )  ;
    }

} // TestVertexClustering


int UnitTestVertexClustering(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering);
}
