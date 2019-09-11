//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/SplitSharpEdges.h>
#include <vtkm/worklet/SurfaceNormals.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

using NormalsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

const vtkm::Vec3f expectedCoords[24] = {
  { 0.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 },
  { 1.0, 1.0, 0.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 },
  { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 0.0 }, { 1.0, 0.0, 1.0 }, { 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 },
  { 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0 }, { 0.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 }, { 1.0, 1.0, 0.0 },
  { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }, { 0.0, 1.0, 1.0 }
};

const std::vector<vtkm::FloatDefault> expectedPointvar{ 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f,
                                                        70.3f, 80.3f, 10.1f, 10.1f, 20.1f, 20.1f,
                                                        30.2f, 30.2f, 40.2f, 40.2f, 50.3f, 50.3f,
                                                        60.3f, 60.3f, 70.3f, 70.3f, 80.3f, 80.3f };

const std::vector<vtkm::Id> expectedConnectivityArray91{ 0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6,
                                                         3, 0, 4, 7, 4, 5, 6, 7, 0, 3, 2, 1 };

vtkm::cont::DataSet Make3DExplicitSimpleCube()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  const int nVerts = 8;
  const int nCells = 6;
  using CoordType = vtkm::Vec3f;
  std::vector<CoordType> coords = {
    CoordType(0, 0, 0), // 0
    CoordType(1, 0, 0), // 1
    CoordType(1, 0, 1), // 2
    CoordType(0, 0, 1), // 3
    CoordType(0, 1, 0), // 4
    CoordType(1, 1, 0), // 5
    CoordType(1, 1, 1), // 6
    CoordType(0, 1, 1)  // 7
  };

  //Connectivity
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numIndices;
  for (size_t i = 0; i < 6; i++)
  {
    shapes.push_back(vtkm::CELL_SHAPE_QUAD);
    numIndices.push_back(4);
  }


  std::vector<vtkm::Id> conn;
  // Down face
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);
  conn.push_back(4);
  // Right face
  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);
  // Top face
  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);
  conn.push_back(6);
  // Left face
  conn.push_back(3);
  conn.push_back(0);
  conn.push_back(4);
  conn.push_back(7);
  // Front face
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);
  // Back face
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(2);
  conn.push_back(1);

  //Create the dataset.
  dataSet = dsb.Create(coords, shapes, numIndices, conn, "coordinates");

  vtkm::FloatDefault vars[nVerts] = { 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f, 70.3f, 80.3f };
  vtkm::FloatDefault cellvar[nCells] = { 100.1f, 200.2f, 300.3f, 400.4f, 500.5f, 600.6f };

  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointvar", vars, nVerts);
  dsf.AddCellField(dataSet, "cellvar", cellvar, nCells);

  return dataSet;
}

void TestSplitSharpEdgesSplitEveryEdge(vtkm::cont::DataSet& simpleCube,
                                       NormalsArrayHandle& faceNormals,
                                       vtkm::worklet::SplitSharpEdges& splitSharpEdges)

{ // Split every edge
  vtkm::FloatDefault featureAngle = 89.0;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> newCoords;
  vtkm::cont::CellSetExplicit<> newCellset;

  splitSharpEdges.Run(simpleCube.GetCellSet(),
                      featureAngle,
                      faceNormals,
                      simpleCube.GetCoordinateSystem().GetData(),
                      newCoords,
                      newCellset);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> pointvar;
  simpleCube.GetPointField("pointvar").GetData().CopyTo(pointvar);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> newPointFields =
    splitSharpEdges.ProcessPointField(pointvar);
  VTKM_TEST_ASSERT(newCoords.GetNumberOfValues() == 24,
                   "new coordinates"
                   " number is wrong");

  auto newCoordsP = newCoords.GetPortalConstControl();
  for (vtkm::Id i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[vtkm::IdComponent(i)][0]),
                     "result value does not match expected value");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[vtkm::IdComponent(i)][1]),
                     "result value does not match expected value");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[vtkm::IdComponent(i)][2]),
                     "result value does not match expected value");
  }

  auto newPointFieldsPortal = newPointFields.GetPortalConstControl();
  for (int i = 0; i < newPointFields.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(
      test_equal(newPointFieldsPortal.Get(i), expectedPointvar[static_cast<unsigned long>(i)]),
      "point field array result does not match expected value");
  }
}

void TestSplitSharpEdgesNoSplit(vtkm::cont::DataSet& simpleCube,
                                NormalsArrayHandle& faceNormals,
                                vtkm::worklet::SplitSharpEdges& splitSharpEdges)

{ // Do nothing
  vtkm::FloatDefault featureAngle = 91.0;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> newCoords;
  vtkm::cont::CellSetExplicit<> newCellset;

  splitSharpEdges.Run(simpleCube.GetCellSet(),
                      featureAngle,
                      faceNormals,
                      simpleCube.GetCoordinateSystem().GetData(),
                      newCoords,
                      newCellset);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> pointvar;
  simpleCube.GetPointField("pointvar").GetData().CopyTo(pointvar);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> newPointFields =
    splitSharpEdges.ProcessPointField(pointvar);
  VTKM_TEST_ASSERT(newCoords.GetNumberOfValues() == 8,
                   "new coordinates"
                   " number is wrong");

  auto newCoordsP = newCoords.GetPortalConstControl();
  for (int i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                     "result value does not match expected value");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                     "result value does not match expected value");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                     "result value does not match expected value");
  }

  const auto& connectivityArray = newCellset.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                                  vtkm::TopologyElementTagPoint());
  auto connectivityArrayPortal = connectivityArray.GetPortalConstControl();
  for (int i = 0; i < connectivityArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(connectivityArrayPortal.Get(i),
                                expectedConnectivityArray91[static_cast<unsigned long>(i)]),
                     "connectivity array result does not match expected value");
  }

  auto newPointFieldsPortal = newPointFields.GetPortalConstControl();
  for (int i = 0; i < newPointFields.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(
      test_equal(newPointFieldsPortal.Get(i), expectedPointvar[static_cast<unsigned long>(i)]),
      "point field array result does not match expected value");
  }
}

void TestSplitSharpEdges()
{
  vtkm::cont::DataSet simpleCube = Make3DExplicitSimpleCube();
  NormalsArrayHandle faceNormals;
  vtkm::worklet::FacetedSurfaceNormals faceted;
  faceted.Run(simpleCube.GetCellSet(), simpleCube.GetCoordinateSystem().GetData(), faceNormals);

  vtkm::worklet::SplitSharpEdges splitSharpEdges;

  TestSplitSharpEdgesSplitEveryEdge(simpleCube, faceNormals, splitSharpEdges);
  TestSplitSharpEdgesNoSplit(simpleCube, faceNormals, splitSharpEdges);
}

} // anonymous namespace

int UnitTestSplitSharpEdges(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSplitSharpEdges, argc, argv);
}
