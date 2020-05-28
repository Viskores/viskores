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

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/testing/MakeTestDataSet.h>

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

  auto newCoords = result.GetCoordinateSystem().GetDataAsMultiplexer();
  auto newCoordsP = newCoords.ReadPortal();
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

  auto newPointvarFieldPortal = newPointvarField.ReadPortal();
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

  auto newCoords = result.GetCoordinateSystem().GetDataAsMultiplexer();
  vtkm::cont::CellSetExplicit<>& newCellset =
    result.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
  auto newCoordsP = newCoords.ReadPortal();
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
  auto connectivityArrayPortal = connectivityArray.ReadPortal();
  for (vtkm::IdComponent i = 0; i < connectivityArray.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(connectivityArrayPortal.Get(static_cast<vtkm::Id>(i)) ==
                       expectedConnectivityArray91[static_cast<unsigned long>(i)],
                     "connectivity array result does not match expected value");
  }

  auto newPointvarFieldPortal = newPointvarField.ReadPortal();
  for (vtkm::IdComponent i = 0; i < newPointvarField.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(test_equal(newPointvarFieldPortal.Get(static_cast<vtkm::Id>(i)),
                                expectedPointvar[static_cast<unsigned long>(i)]),
                     "point field array result does not match expected value");
  }
}

void TestWithExplicitData()
{
  vtkm::filter::testing::MakeTestDataSet makeTestData;
  vtkm::cont::DataSet simpleCube = makeTestData.Make3DExplicitSimpleCube();
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
  dataSet = split.Execute(dataSet);
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
