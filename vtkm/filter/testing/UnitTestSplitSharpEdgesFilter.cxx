//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/CellAverage.h>
#include <vtkm/filter/Contour.h>
#include <vtkm/filter/SplitSharpEdges.h>
#include <vtkm/filter/SurfaceNormals.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/source/Wavelet.h>

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

const std::vector<vtkm::Id> expectedConnectivityArray91{ 0, 1, 5, 4, 1, 2, 6, 5, 2, 3, 7, 6,
                                                         3, 0, 4, 7, 4, 5, 6, 7, 0, 3, 2, 1 };
const std::vector<vtkm::FloatDefault> expectedPointvar{ 10.1f, 20.1f, 30.2f, 40.2f, 50.3f, 60.3f,
                                                        70.3f, 80.3f, 10.1f, 10.1f, 20.1f, 20.1f,
                                                        30.2f, 30.2f, 40.2f, 40.2f, 50.3f, 50.3f,
                                                        60.3f, 60.3f, 70.3f, 70.3f, 80.3f, 80.3f };


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

vtkm::cont::DataSet Make3DWavelet()
{

  vtkm::source::Wavelet wavelet({ -25 }, { 25 });
  wavelet.SetFrequency({ 60, 30, 40 });
  wavelet.SetMagnitude({ 5 });

  vtkm::cont::DataSet result = wavelet.Execute();
  return result;
}


void TestSplitSharpEdgesFilterSplitEveryEdge(vtkm::cont::DataSet& simpleCubeWithSN,
                                             vtkm::filter::SplitSharpEdges& splitSharpEdgesFilter)
{
  // Split every edge
  vtkm::FloatDefault featureAngle = 89.0;
  splitSharpEdgesFilter.SetFeatureAngle(featureAngle);
  splitSharpEdgesFilter.SetActiveField("Normals", vtkm::cont::Field::Association::CELL_SET);
  vtkm::cont::DataSet result = splitSharpEdgesFilter.Execute(simpleCubeWithSN);

  auto newCoords = result.GetCoordinateSystem().GetData();
  auto newCoordsP = newCoords.GetPortalConstControl();
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> newPointvarField;
  result.GetField("pointvar").GetData().CopyTo(newPointvarField);

  for (vtkm::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[0], expectedCoords[i][0]),
                     "result value does not match expected value");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[1], expectedCoords[i][1]),
                     "result value does not match expected value");
    VTKM_TEST_ASSERT(test_equal(newCoordsP.Get(i)[2], expectedCoords[i][2]),
                     "result value does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.GetPortalConstControl();
  for (vtkm::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<vtkm::Id>(i)),
                                expectedPointvar[static_cast<unsigned long>(i)]),
                     "point field array result does not match expected value");
  }
}

void TestSplitSharpEdgesFilterNoSplit(vtkm::cont::DataSet& simpleCubeWithSN,
                                      vtkm::filter::SplitSharpEdges& splitSharpEdgesFilter)
{
  // Do nothing
  vtkm::FloatDefault featureAngle = 91.0;
  splitSharpEdgesFilter.SetFeatureAngle(featureAngle);
  splitSharpEdgesFilter.SetActiveField("Normals", vtkm::cont::Field::Association::CELL_SET);
  vtkm::cont::DataSet result = splitSharpEdgesFilter.Execute(simpleCubeWithSN);

  auto newCoords = result.GetCoordinateSystem().GetData();
  vtkm::cont::CellSetExplicit<>& newCellset =
    result.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
  auto newCoordsP = newCoords.GetPortalConstControl();
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> newPointvarField;
  result.GetField("pointvar").GetData().CopyTo(newPointvarField);

  for (vtkm::IdComponent i = 0; i < newCoords.GetNumberOfValues(); i++)
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
  for (vtkm::IdComponent i = 0; i < connectivityArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(connectivityArrayPortal.Get(static_cast<vtkm::Id>(i)) ==
                       expectedConnectivityArray91[static_cast<unsigned long>(i)],
                     "connectivity array result does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.GetPortalConstControl();
  for (vtkm::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<vtkm::Id>(i)),
                                expectedPointvar[static_cast<unsigned long>(i)]),
                     "point field array result does not match expected value");
  }
}

void TestWithExplicitData()
{
  vtkm::cont::DataSet simpleCube = Make3DExplicitSimpleCube();
  // Generate surface normal field
  vtkm::filter::SurfaceNormals surfaceNormalsFilter;
  surfaceNormalsFilter.SetGenerateCellNormals(true);
  vtkm::cont::DataSet simpleCubeWithSN = surfaceNormalsFilter.Execute(simpleCube);
  VTKM_TEST_ASSERT(simpleCubeWithSN.HasCellField("Normals"), "Cell normals missing.");
  VTKM_TEST_ASSERT(simpleCubeWithSN.HasPointField("pointvar"), "point field pointvar missing.");


  vtkm::filter::SplitSharpEdges splitSharpEdgesFilter;

  TestSplitSharpEdgesFilterSplitEveryEdge(simpleCubeWithSN, splitSharpEdgesFilter);
  TestSplitSharpEdgesFilterNoSplit(simpleCubeWithSN, splitSharpEdgesFilter);
}

struct SplitSharpTestPolicy : public vtkm::filter::PolicyBase<SplitSharpTestPolicy>
{
  using StructuredCellSetList = vtkm::List<vtkm::cont::CellSetStructured<3>>;
  using UnstructuredCellSetList = vtkm::List<vtkm::cont::CellSetSingleType<>>;
  using AllCellSetList = vtkm::ListAppend<StructuredCellSetList, UnstructuredCellSetList>;
  using FieldTypeList = vtkm::List<vtkm::FloatDefault, vtkm::Vec3f>;
};


void TestWithStructuredData()
{
  // Generate a wavelet:
  vtkm::cont::DataSet dataSet = Make3DWavelet();

  // Cut a contour:
  vtkm::filter::Contour contour;
  contour.SetActiveField("scalars", vtkm::cont::Field::Association::POINTS);
  contour.SetNumberOfIsoValues(1);
  contour.SetIsoValue(192);
  contour.SetMergeDuplicatePoints(true);
  contour.SetGenerateNormals(true);
  contour.SetComputeFastNormalsForStructured(true);
  contour.SetNormalArrayName("normals");
  dataSet = contour.Execute(dataSet);

  // Compute cell normals:
  vtkm::filter::CellAverage cellNormals;
  cellNormals.SetActiveField("normals", vtkm::cont::Field::Association::POINTS);
  dataSet = cellNormals.Execute(dataSet);

  // Split sharp edges:
  std::cout << dataSet.GetNumberOfCells() << std::endl;
  std::cout << dataSet.GetNumberOfPoints() << std::endl;
  vtkm::filter::SplitSharpEdges split;
  split.SetActiveField("normals", vtkm::cont::Field::Association::CELL_SET);
  dataSet = split.Execute(dataSet, SplitSharpTestPolicy{});
}


void TestSplitSharpEdgesFilter()
{
  TestWithExplicitData();
  TestWithStructuredData();
}

} // anonymous namespace

int UnitTestSplitSharpEdgesFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestSplitSharpEdgesFilter, argc, argv);
}
