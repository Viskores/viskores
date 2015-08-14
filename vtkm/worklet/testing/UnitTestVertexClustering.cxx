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

template<typename T, vtkm::IdComponent N>
vtkm::cont::ArrayHandle<T> copyFromVec( vtkm::cont::ArrayHandle< vtkm::Vec<T, N> > const& other)
{
    const T *vmem = reinterpret_cast< const T *>(& *other.GetPortalConstControl().GetIteratorBegin());
    vtkm::cont::ArrayHandle<T> mem = vtkm::cont::make_ArrayHandle(vmem, other.GetNumberOfValues()*N);
    vtkm::cont::ArrayHandle<T> result;
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(mem,result);
    return result;
}

template<typename T, typename StorageTag>
vtkm::cont::ArrayHandle<T> copyFromImplicit( vtkm::cont::ArrayHandle<T, StorageTag> const& other)
{
  vtkm::cont::ArrayHandle<T> result;
  vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(other, result);
  return result;
}

vtkm::cont::DataSet RunVertexClustering(vtkm::cont::DataSet &dataSet,
                                        const vtkm::Float64 bounds[6],
                                        vtkm::Id nDivisions)
{
  typedef vtkm::Vec<vtkm::Float32,3>  PointType;

  // TODO: The VertexClustering operation needs some work. You should not have
  // to cast the cell set yourself (I don't think).
  vtkm::cont::CellSetExplicit<> &cellSet =
      dataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();

  vtkm::cont::ArrayHandle<PointType> pointArray = dataSet.GetField("xyz").GetData().CastToArrayHandle<PointType, VTKM_DEFAULT_STORAGE_TAG>();
  vtkm::cont::ArrayHandle<vtkm::Id> pointIdArray = cellSet.GetConnectivityArray();
  vtkm::cont::ArrayHandle<vtkm::Id> indexOffsetArray = cellSet.GetIndexOffsetArray();

  vtkm::cont::ArrayHandle<PointType> output_pointArray ;
  vtkm::cont::ArrayHandle<vtkm::Id3> output_pointId3Array ;

  // run
  vtkm::worklet::VertexClustering<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().run(
        pointArray,
        pointIdArray,
        indexOffsetArray,
        bounds,
        nDivisions,
        output_pointArray,
        output_pointId3Array);

  vtkm::cont::DataSet newDataSet;

  newDataSet.AddField(vtkm::cont::Field("xyz", 0, vtkm::cont::Field::ASSOC_POINTS, output_pointArray));
  newDataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("xyz"));

  vtkm::Id cells = output_pointId3Array.GetNumberOfValues();
  if (cells > 0)
  {
    //typedef typename vtkm::cont::ArrayHandleConstant<vtkm::Id>::StorageTag ConstantStorage;
    //typedef typename vtkm::cont::ArrayHandleImplicit<vtkm::Id, CounterOfThree>::StorageTag CountingStorage;

    vtkm::cont::CellSetExplicit<> newCellSet("cells", 0);

    newCellSet.Fill(
          copyFromImplicit(vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(vtkm::VTKM_TRIANGLE, cells)),
          copyFromImplicit(vtkm::cont::make_ArrayHandleConstant<vtkm::Id>(3, cells)),
          copyFromVec(output_pointId3Array)
          );

    newDataSet.AddCellSet(newCellSet);
  }

  return newDataSet;
}

void TestVertexClustering()
{
  vtkm::Float64 bounds[6];
  const vtkm::Id divisions = 3;
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet dataSet = maker.Make3DExplicitDataSetCowNose(bounds);

  // run
  vtkm::cont::DataSet outDataSet = RunVertexClustering(dataSet, bounds, divisions);

  // test
  const vtkm::Id output_pointIds = 9;
  vtkm::Id output_pointId[output_pointIds] = {0,1,3, 1,5,4, 1,2,5};
  const vtkm::Id output_points = 6;
  double output_point[output_points][3] = {{0.0174716003,0.0501927994,0.0930275023}, {0.0320714004,0.14704667,0.0952706337}, {0.0268670674,0.246195346,0.119720004}, {0.00215422804,0.0340906903,0.180881709}, {0.0108188,0.152774006,0.167914003}, {0.0202241503,0.225427493,0.140208006}};

  VTKM_TEST_ASSERT(outDataSet.GetNumberOfFields() == 1, "Number of output fields mismatch");
  typedef vtkm::Vec<vtkm::Float32, 3> PointType;
  typedef vtkm::cont::ArrayHandle<PointType > PointArray;
  PointArray pointArray = outDataSet.GetField(0).GetData().CastToArrayHandle<PointArray::ValueType, PointArray::StorageTag>();
  VTKM_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points, "Number of output points mismatch" );
  for (vtkm::Id i = 0; i < pointArray.GetNumberOfValues(); ++i)
    {
      const PointType &p1 = pointArray.GetPortalConstControl().Get(i);
      PointType p2 = vtkm::make_Vec<vtkm::Float32>((vtkm::Float32)output_point[i][0], (vtkm::Float32)output_point[i][1], (vtkm::Float32)output_point[i][2]) ;
      std::cout << "point: " << p1 << " " << p2 << std::endl;
      VTKM_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
    }

  VTKM_TEST_ASSERT(outDataSet.GetNumberOfCellSets() == 1, "Number of output cellsets mismatch");
  vtkm::cont::CellSetExplicit<> &cellSet =
      outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetExplicit<> >();
  VTKM_TEST_ASSERT(
        cellSet.GetConnectivityArray().GetNumberOfValues() == output_pointIds,
        "Number of connectivity array elements mismatch");
  for (vtkm::Id i=0; i<cellSet.GetConnectivityArray().GetNumberOfValues(); i++)
    {
      vtkm::Id id1 = cellSet.GetConnectivityArray().GetPortalConstControl().Get(i) ;
      vtkm::Id id2 = output_pointId[i] ;
      std::cout << "pointid: " << id1 << " " << id2 << std::endl;
      //VTKM_TEST_ASSERT( id1 == id2, "Connectivity Array mismatch" )  ;
    }

} // TestVertexClustering


int UnitTestVertexClustering(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering);
}
