//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellSetPermutation.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace
{

struct WorkletPointToCell : public vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell numPoints);
  using ExecutionSignature = void(PointIndices, _2);
  using InputDomain = _1;

  template <typename PointIndicesType>
  VTKM_EXEC void operator()(const PointIndicesType& pointIndices, vtkm::Id& numPoints) const
  {
    numPoints = pointIndices.GetNumberOfComponents();
  }
};

struct WorkletCellToPoint : public vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldOutPoint numCells);
  using ExecutionSignature = void(CellIndices, _2);
  using InputDomain = _1;

  template <typename CellIndicesType>
  VTKM_EXEC void operator()(const CellIndicesType& cellIndices, vtkm::Id& numCells) const
  {
    numCells = cellIndices.GetNumberOfComponents();
  }
};

struct CellsOfPoint : public vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn cellset, FieldInPoint offset, WholeArrayOut cellIds);
  using ExecutionSignature = void(CellIndices, _2, _3);
  using InputDomain = _1;

  template <typename CellIndicesType, typename CellIdsPortal>
  VTKM_EXEC void operator()(const CellIndicesType& cellIndices,
                            vtkm::Id offset,
                            const CellIdsPortal& out) const
  {
    vtkm::IdComponent count = cellIndices.GetNumberOfComponents();
    for (vtkm::IdComponent i = 0; i < count; ++i)
    {
      out.Set(offset++, cellIndices[i]);
    }
  }
};

template <typename CellSetType, typename PermutationArrayHandleType>
std::vector<vtkm::Id> ComputeCellToPointExpected(const CellSetType& cellset,
                                                 const PermutationArrayHandleType& permutation)
{
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, numIndices);
  std::cout << "\n";

  vtkm::cont::ArrayHandle<vtkm::Id> indexOffsets;
  vtkm::Id connectivityLength = vtkm::cont::Algorithm::ScanExclusive(numIndices, indexOffsets);

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  connectivity.Allocate(connectivityLength);
  vtkm::worklet::DispatcherMapTopology<CellsOfPoint>().Invoke(cellset, indexOffsets, connectivity);

  std::vector<bool> permutationMask(static_cast<std::size_t>(cellset.GetNumberOfCells()), false);
  for (vtkm::Id i = 0; i < permutation.GetNumberOfValues(); ++i)
  {
    permutationMask[static_cast<std::size_t>(permutation.GetPortalConstControl().Get(i))] = true;
  }

  vtkm::Id numberOfPoints = cellset.GetNumberOfPoints();
  std::vector<vtkm::Id> expected(static_cast<std::size_t>(numberOfPoints), 0);
  for (vtkm::Id i = 0; i < numberOfPoints; ++i)
  {
    vtkm::Id offset = indexOffsets.GetPortalConstControl().Get(i);
    vtkm::Id count = numIndices.GetPortalConstControl().Get(i);
    for (vtkm::Id j = 0; j < count; ++j)
    {
      vtkm::Id cellId = connectivity.GetPortalConstControl().Get(offset++);
      if (permutationMask[static_cast<std::size_t>(cellId)])
      {
        ++expected[static_cast<std::size_t>(i)];
      }
    }
  }

  return expected;
}

template <typename CellSetType>
vtkm::cont::CellSetPermutation<CellSetType, vtkm::cont::ArrayHandleCounting<vtkm::Id>> TestCellSet(
  const CellSetType& cellset)
{
  vtkm::Id numberOfCells = cellset.GetNumberOfCells() / 2;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> permutation(0, 2, numberOfCells);
  auto cs = vtkm::cont::make_CellSetPermutation(permutation, cellset);
  vtkm::cont::ArrayHandle<vtkm::Id> result;

  std::cout << "\t\tTesting PointToCell\n";
  vtkm::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cs, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == numberOfCells,
                   "result length not equal to number of cells");
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) ==
                       cellset.GetNumberOfPointsInCell(permutation.GetPortalConstControl().Get(i)),
                     "incorrect result");
  }

  std::cout << "\t\tTesting CellToPoint\n";
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cs, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");
  auto expected = ComputeCellToPointExpected(cellset, permutation);
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == expected[static_cast<std::size_t>(i)],
                     "incorrect result");
  }
  std::cout << "Testing resource releasing in CellSetPermutation:\n";
  cs.ReleaseResourcesExecution();
  VTKM_TEST_ASSERT(cs.GetNumberOfCells() == cellset.GetNumberOfCells() / 2,
                   "release execution resources should not change the number of cells");
  VTKM_TEST_ASSERT(cs.GetNumberOfPoints() == cellset.GetNumberOfPoints(),
                   "release execution resources should not change the number of points");

  return cs;
}

template <typename CellSetType>
void RunTests(const CellSetType& cellset)
{
  std::cout << "\tTesting CellSetPermutation:\n";
  auto p1 = TestCellSet(cellset);
  std::cout << "\tTesting CellSetPermutation of CellSetPermutation:\n";
  TestCellSet(p1);
  std::cout << "----------------------------------------------------------\n";
}

void TestCellSetPermutation()
{
  vtkm::cont::DataSet dataset;
  vtkm::cont::testing::MakeTestDataSet maker;

  std::cout << "Testing CellSetStructured<2>\n";
  dataset = maker.Make2DUniformDataSet1();
  RunTests(dataset.GetCellSet().Cast<vtkm::cont::CellSetStructured<2>>());

  std::cout << "Testing CellSetStructured<3>\n";
  dataset = maker.Make3DUniformDataSet1();
  RunTests(dataset.GetCellSet().Cast<vtkm::cont::CellSetStructured<3>>());

  std::cout << "Testing CellSetExplicit\n";
  dataset = maker.Make3DExplicitDataSetPolygonal();
  RunTests(dataset.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>());

  std::cout << "Testing CellSetSingleType\n";
  dataset = maker.Make3DExplicitDataSetCowNose();
  RunTests(dataset.GetCellSet().Cast<vtkm::cont::CellSetSingleType<>>());
}

} // anonymous namespace

int UnitTestCellSetPermutation(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetPermutation, argc, argv);
}
