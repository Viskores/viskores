//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  const vtkm::Id3 divisions(3, 3, 3);
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet dataSet = maker.Make3DExplicitDataSetCowNose();

  //compute the bounds before calling the algorithm
  vtkm::Bounds bounds = dataSet.GetCoordinateSystem().GetBounds();

  // run
  vtkm::worklet::VertexClustering clustering;
  vtkm::cont::DataSet outDataSet =
    clustering.Run(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), bounds, divisions);

  using FieldArrayType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  FieldArrayType pointvar = clustering.ProcessPointField(
    dataSet.GetPointField("pointvar").GetData().Cast<FieldArrayType>());
  FieldArrayType cellvar =
    clustering.ProcessCellField(dataSet.GetCellField("cellvar").GetData().Cast<FieldArrayType>());

  // test
  const vtkm::Id output_pointIds = 18;
  vtkm::Id output_pointId[output_pointIds] = {
    0, 1, 3, 1, 4, 3, 2, 5, 3, 0, 3, 5, 2, 3, 6, 3, 4, 6
  };
  const vtkm::Id output_points = 7;
  vtkm::Float64 output_point[output_points][3] = {
    { 0.0174716, 0.0501928, 0.0930275 }, { 0.0307091, 0.15214200, 0.0539249 },
    { 0.0174172, 0.1371240, 0.1245530 }, { 0.0480879, 0.15187400, 0.1073340 },
    { 0.0180085, 0.2043600, 0.1453160 }, { -.000129414, 0.00247137, 0.1765610 },
    { 0.0108188, 0.1527740, 0.1679140 }
  };

  vtkm::Float32 output_pointvar[output_points] = { 28.f, 19.f, 25.f, 15.f, 16.f, 21.f, 30.f };
  vtkm::Float32 output_cellvar[output_pointIds / 3] = { 145.f, 134.f, 138.f, 140.f, 149.f, 144.f };

  {
    using CellSetType = vtkm::cont::CellSetSingleType<>;
    CellSetType cellSet;
    outDataSet.GetCellSet().CopyTo(cellSet);
    auto cellArray =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    std::cerr << "output_pointIds = " << cellArray.GetNumberOfValues() << "\n";
    std::cerr << "output_pointId[] = ";
    vtkm::cont::printSummary_ArrayHandle(cellArray, std::cerr, true);
  }

  {
    auto pointArray = outDataSet.GetCoordinateSystem(0).GetData();
    std::cerr << "output_points = " << pointArray.GetNumberOfValues() << "\n";
    std::cerr << "output_point[] = ";
    vtkm::cont::printSummary_ArrayHandle(pointArray, std::cerr, true);
  }

  vtkm::cont::printSummary_ArrayHandle(pointvar, std::cerr, true);
  vtkm::cont::printSummary_ArrayHandle(cellvar, std::cerr, true);

  VTKM_TEST_ASSERT(outDataSet.GetNumberOfCoordinateSystems() == 1,
                   "Number of output coordinate systems mismatch");
  using PointType = vtkm::Vec3f_64;
  auto pointArray = outDataSet.GetCoordinateSystem(0).GetData();
  VTKM_TEST_ASSERT(pointArray.GetNumberOfValues() == output_points,
                   "Number of output points mismatch");
  for (vtkm::Id i = 0; i < pointArray.GetNumberOfValues(); ++i)
  {
    const PointType& p1 = pointArray.GetPortalConstControl().Get(i);
    PointType p2 = vtkm::make_Vec(output_point[i][0], output_point[i][1], output_point[i][2]);
    VTKM_TEST_ASSERT(test_equal(p1, p2), "Point Array mismatch");
  }

  using CellSetType = vtkm::cont::CellSetSingleType<>;
  CellSetType cellSet;
  outDataSet.GetCellSet().CopyTo(cellSet);
  VTKM_TEST_ASSERT(
    cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint())
        .GetNumberOfValues() == output_pointIds,
    "Number of connectivity array elements mismatch");
  for (vtkm::Id i = 0; i <
       cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint())
         .GetNumberOfValues();
       i++)
  {
    vtkm::Id id1 =
      cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint())
        .GetPortalConstControl()
        .Get(i);
    vtkm::Id id2 = output_pointId[i];
    VTKM_TEST_ASSERT(id1 == id2, "Connectivity Array mismatch");
  }

  {
    auto portal = pointvar.GetPortalConstControl();
    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == output_points, "Point field size mismatch.");
    for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(portal.Get(i), output_pointvar[i]), "Point field mismatch.");
    }
  }

  {
    auto portal = cellvar.GetPortalConstControl();
    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == output_pointIds / 3,
                     "Cell field size mismatch.");
    for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(test_equal(portal.Get(i), output_cellvar[i]), "Cell field mismatch.");
    }
  }

} // TestVertexClustering

int UnitTestVertexClustering(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestVertexClustering, argc, argv);
}
