//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/CellDeepCopy.h>

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetPermutation.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

vtkm::cont::CellSetExplicit<> CreateCellSet()
{
  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet data = makeData.Make3DExplicitDataSet0();
  vtkm::cont::CellSetExplicit<> cellSet;
  data.GetCellSet().CopyTo(cellSet);
  return cellSet;
}

vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>,
                               vtkm::cont::ArrayHandleCounting<vtkm::Id>>
CreatePermutedCellSet()
{
  std::cout << "Creating input cell set" << std::endl;

  vtkm::cont::CellSetExplicit<> cellSet = CreateCellSet();
  return vtkm::cont::make_CellSetPermutation(
    vtkm::cont::ArrayHandleCounting<vtkm::Id>(
      cellSet.GetNumberOfCells() - 1, -1, cellSet.GetNumberOfCells()),
    cellSet);
}

template <typename CellSetType>
vtkm::cont::CellSetExplicit<> DoCellDeepCopy(const CellSetType& inCells)
{
  std::cout << "Doing cell copy" << std::endl;

  return vtkm::worklet::CellDeepCopy::Run(inCells);
}

void CheckOutput(const vtkm::cont::CellSetExplicit<>& copiedCells)
{
  std::cout << "Checking copied cells" << std::endl;

  vtkm::cont::CellSetExplicit<> originalCells = CreateCellSet();

  vtkm::Id numberOfCells = copiedCells.GetNumberOfCells();
  VTKM_TEST_ASSERT(numberOfCells == originalCells.GetNumberOfCells(),
                   "Result has wrong number of cells");

  // Cells should be copied backward. Check that.
  for (vtkm::Id cellIndex = 0; cellIndex < numberOfCells; cellIndex++)
  {
    vtkm::Id oCellIndex = numberOfCells - cellIndex - 1;
    VTKM_TEST_ASSERT(copiedCells.GetCellShape(cellIndex) == originalCells.GetCellShape(oCellIndex),
                     "Bad cell shape");

    vtkm::IdComponent numPoints = copiedCells.GetNumberOfPointsInCell(cellIndex);
    VTKM_TEST_ASSERT(numPoints == originalCells.GetNumberOfPointsInCell(oCellIndex),
                     "Bad number of points in cell");

    // Only checking 3 points. All cells should have at least 3
    vtkm::Id3 cellPoints{ 0 };
    copiedCells.GetIndices(cellIndex, cellPoints);
    vtkm::Id3 oCellPoints{ 0 };
    originalCells.GetIndices(oCellIndex, oCellPoints);
    VTKM_TEST_ASSERT(cellPoints == oCellPoints, "Point indices not copied correctly");
  }
}

void RunTest()
{
  vtkm::cont::CellSetExplicit<> cellSet = DoCellDeepCopy(CreatePermutedCellSet());
  CheckOutput(cellSet);
}

} // anonymous namespace

int UnitTestCellDeepCopy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RunTest, argc, argv);
}
