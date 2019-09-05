//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/CleanGrid.h>

#include <vtkm/filter/Contour.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestUniformGrid(vtkm::filter::CleanGrid clean)
{
  std::cout << "Testing 'clean' uniform grid." << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;

  vtkm::cont::DataSet inData = makeData.Make2DUniformDataSet0();

  clean.SetFieldsToPass({ "pointvar", "cellvar" });
  vtkm::cont::DataSet outData = clean.Execute(inData);
  VTKM_TEST_ASSERT(outData.HasField("pointvar"), "Failed to map point field");
  VTKM_TEST_ASSERT(outData.HasField("cellvar"), "Failed to map cell field");

  vtkm::cont::CellSetExplicit<> outCellSet;
  outData.GetCellSet().CopyTo(outCellSet);
  VTKM_TEST_ASSERT(outCellSet.GetNumberOfPoints() == 6,
                   "Wrong number of points: ",
                   outCellSet.GetNumberOfPoints());
  VTKM_TEST_ASSERT(
    outCellSet.GetNumberOfCells() == 2, "Wrong number of cells: ", outCellSet.GetNumberOfCells());
  vtkm::Id4 cellIds;
  outCellSet.GetIndices(0, cellIds);
  VTKM_TEST_ASSERT((cellIds == vtkm::Id4(0, 1, 4, 3)), "Bad cell ids: ", cellIds);
  outCellSet.GetIndices(1, cellIds);
  VTKM_TEST_ASSERT((cellIds == vtkm::Id4(1, 2, 5, 4)), "Bad cell ids: ", cellIds);

  vtkm::cont::ArrayHandle<vtkm::Float32> outPointField;
  outData.GetField("pointvar").GetData().CopyTo(outPointField);
  VTKM_TEST_ASSERT(outPointField.GetNumberOfValues() == 6,
                   "Wrong point field size: ",
                   outPointField.GetNumberOfValues());
  VTKM_TEST_ASSERT(test_equal(outPointField.GetPortalConstControl().Get(1), 20.1),
                   "Bad point field value: ",
                   outPointField.GetPortalConstControl().Get(1));
  VTKM_TEST_ASSERT(test_equal(outPointField.GetPortalConstControl().Get(4), 50.1),
                   "Bad point field value: ",
                   outPointField.GetPortalConstControl().Get(1));

  vtkm::cont::ArrayHandle<vtkm::Float32> outCellField;
  outData.GetField("cellvar").GetData().CopyTo(outCellField);
  VTKM_TEST_ASSERT(outCellField.GetNumberOfValues() == 2, "Wrong cell field size.");
  VTKM_TEST_ASSERT(test_equal(outCellField.GetPortalConstControl().Get(0), 100.1),
                   "Bad cell field value",
                   outCellField.GetPortalConstControl().Get(0));
  VTKM_TEST_ASSERT(test_equal(outCellField.GetPortalConstControl().Get(1), 200.1),
                   "Bad cell field value",
                   outCellField.GetPortalConstControl().Get(0));
}

void TestPointMerging()
{
  vtkm::cont::testing::MakeTestDataSet makeDataSet;
  vtkm::cont::DataSet baseData = makeDataSet.Make3DUniformDataSet3(vtkm::Id3(4, 4, 4));

  vtkm::filter::Contour marchingCubes;
  marchingCubes.SetIsoValue(0.05);
  marchingCubes.SetMergeDuplicatePoints(false);
  marchingCubes.SetActiveField("pointvar");
  vtkm::cont::DataSet inData = marchingCubes.Execute(baseData);
  constexpr vtkm::Id originalNumPoints = 228;
  constexpr vtkm::Id originalNumCells = 76;
  VTKM_TEST_ASSERT(inData.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  VTKM_TEST_ASSERT(inData.GetNumberOfCells() == originalNumCells);

  vtkm::filter::CleanGrid cleanGrid;

  std::cout << "Clean grid without any merging" << std::endl;
  cleanGrid.SetCompactPointFields(false);
  cleanGrid.SetMergePoints(false);
  cleanGrid.SetRemoveDegenerateCells(false);
  vtkm::cont::DataSet noMerging = cleanGrid.Execute(inData);
  VTKM_TEST_ASSERT(noMerging.GetNumberOfCells() == originalNumCells);
  VTKM_TEST_ASSERT(noMerging.GetCellSet().GetNumberOfPoints() == originalNumPoints);
  VTKM_TEST_ASSERT(noMerging.GetNumberOfPoints() == originalNumPoints);
  VTKM_TEST_ASSERT(noMerging.GetField("pointvar").GetNumberOfValues() == originalNumPoints);
  VTKM_TEST_ASSERT(noMerging.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points" << std::endl;
  cleanGrid.SetMergePoints(true);
  cleanGrid.SetFastMerge(false);
  vtkm::cont::DataSet closeMerge = cleanGrid.Execute(inData);
  constexpr vtkm::Id closeMergeNumPoints = 62;
  VTKM_TEST_ASSERT(closeMerge.GetNumberOfCells() == originalNumCells);
  VTKM_TEST_ASSERT(closeMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  VTKM_TEST_ASSERT(closeMerge.GetNumberOfPoints() == closeMergeNumPoints);
  VTKM_TEST_ASSERT(closeMerge.GetField("pointvar").GetNumberOfValues() == closeMergeNumPoints);
  VTKM_TEST_ASSERT(closeMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid by merging very close points with fast merge" << std::endl;
  cleanGrid.SetFastMerge(true);
  vtkm::cont::DataSet closeFastMerge = cleanGrid.Execute(inData);
  VTKM_TEST_ASSERT(closeFastMerge.GetNumberOfCells() == originalNumCells);
  VTKM_TEST_ASSERT(closeFastMerge.GetCellSet().GetNumberOfPoints() == closeMergeNumPoints);
  VTKM_TEST_ASSERT(closeFastMerge.GetNumberOfPoints() == closeMergeNumPoints);
  VTKM_TEST_ASSERT(closeFastMerge.GetField("pointvar").GetNumberOfValues() == closeMergeNumPoints);
  VTKM_TEST_ASSERT(closeFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points" << std::endl;
  cleanGrid.SetFastMerge(false);
  cleanGrid.SetTolerance(0.1);
  vtkm::cont::DataSet farMerge = cleanGrid.Execute(inData);
  constexpr vtkm::Id farMergeNumPoints = 36;
  VTKM_TEST_ASSERT(farMerge.GetNumberOfCells() == originalNumCells);
  VTKM_TEST_ASSERT(farMerge.GetCellSet().GetNumberOfPoints() == farMergeNumPoints);
  VTKM_TEST_ASSERT(farMerge.GetNumberOfPoints() == farMergeNumPoints);
  VTKM_TEST_ASSERT(farMerge.GetField("pointvar").GetNumberOfValues() == farMergeNumPoints);
  VTKM_TEST_ASSERT(farMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly" << std::endl;
  cleanGrid.SetFastMerge(true);
  vtkm::cont::DataSet farFastMerge = cleanGrid.Execute(inData);
  constexpr vtkm::Id farFastMergeNumPoints = 19;
  VTKM_TEST_ASSERT(farFastMerge.GetNumberOfCells() == originalNumCells);
  VTKM_TEST_ASSERT(farFastMerge.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  VTKM_TEST_ASSERT(farFastMerge.GetNumberOfPoints() == farFastMergeNumPoints);
  VTKM_TEST_ASSERT(farFastMerge.GetField("pointvar").GetNumberOfValues() == farFastMergeNumPoints);
  VTKM_TEST_ASSERT(farFastMerge.GetField("cellvar").GetNumberOfValues() == originalNumCells);

  std::cout << "Clean grid with largely separated points quickly with degenerate cells"
            << std::endl;
  cleanGrid.SetRemoveDegenerateCells(true);
  vtkm::cont::DataSet noDegenerateCells = cleanGrid.Execute(inData);
  constexpr vtkm::Id numNonDegenerateCells = 33;
  VTKM_TEST_ASSERT(noDegenerateCells.GetNumberOfCells() == numNonDegenerateCells);
  VTKM_TEST_ASSERT(noDegenerateCells.GetCellSet().GetNumberOfPoints() == farFastMergeNumPoints);
  VTKM_TEST_ASSERT(noDegenerateCells.GetNumberOfPoints() == farFastMergeNumPoints);
  VTKM_TEST_ASSERT(noDegenerateCells.GetField("pointvar").GetNumberOfValues() ==
                   farFastMergeNumPoints);
  VTKM_TEST_ASSERT(noDegenerateCells.GetField("cellvar").GetNumberOfValues() ==
                   numNonDegenerateCells);
}

void RunTest()
{
  vtkm::filter::CleanGrid clean;

  std::cout << "*** Test with compact point fields on merge points off" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points off" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields on merge points on" << std::endl;
  clean.SetCompactPointFields(true);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test with compact point fields off merge points on" << std::endl;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(true);
  TestUniformGrid(clean);

  std::cout << "*** Test point merging" << std::endl;
  TestPointMerging();
}

} // anonymous namespace

int UnitTestCleanGrid(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RunTest, argc, argv);
}
