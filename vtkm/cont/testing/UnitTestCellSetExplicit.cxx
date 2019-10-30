//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace
{

using CellTag = vtkm::TopologyElementTagCell;
using PointTag = vtkm::TopologyElementTagPoint;

const vtkm::Id numberOfPoints = 11;

const vtkm::UInt8 g_shapes[] = { static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_HEXAHEDRON),
                                 static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_PYRAMID),
                                 static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_TETRA),
                                 static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_WEDGE) };
const vtkm::UInt8 g_shapes2[] = { g_shapes[1], g_shapes[2] };

const vtkm::Id g_offsets[] = { 0, 8, 13, 17, 23 };
const vtkm::Id g_offsets2[] = { 0, 5, 9 };

const vtkm::Id g_connectivity[] = { 0, 1, 5, 4,  3, 2, 6, 7, 1, 5, 6, 2,
                                    8, 5, 8, 10, 6, 4, 7, 9, 5, 6, 10 };
const vtkm::Id g_connectivity2[] = { 1, 5, 6, 2, 8, 5, 8, 10, 6 };

template <typename T, std::size_t Length>
vtkm::Id ArrayLength(const T (&)[Length])
{
  return static_cast<vtkm::Id>(Length);
}

// all points are part of atleast 1 cell
vtkm::cont::CellSetExplicit<> MakeTestCellSet1()
{
  vtkm::cont::CellSetExplicit<> cs;
  cs.Fill(numberOfPoints,
          vtkm::cont::make_ArrayHandle(g_shapes, ArrayLength(g_shapes)),
          vtkm::cont::make_ArrayHandle(g_connectivity, ArrayLength(g_connectivity)),
          vtkm::cont::make_ArrayHandle(g_offsets, ArrayLength(g_offsets)));
  return cs;
}

// some points are not part of any cell
vtkm::cont::CellSetExplicit<> MakeTestCellSet2()
{
  vtkm::cont::CellSetExplicit<> cs;
  cs.Fill(numberOfPoints,
          vtkm::cont::make_ArrayHandle(g_shapes2, ArrayLength(g_shapes2)),
          vtkm::cont::make_ArrayHandle(g_connectivity2, ArrayLength(g_connectivity2)),
          vtkm::cont::make_ArrayHandle(g_offsets2, ArrayLength(g_offsets2)));
  return cs;
}

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

void TestCellSetExplicit()
{
  vtkm::cont::CellSetExplicit<> cellset;
  vtkm::cont::ArrayHandle<vtkm::Id> result;

  std::cout << "----------------------------------------------------\n";
  std::cout << "Testing Case 1 (all points are part of atleast 1 cell): \n";
  cellset = MakeTestCellSet1();

  std::cout << "\tTesting PointToCell\n";
  vtkm::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cellset, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfCells(),
                   "result length not equal to number of cells");
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == cellset.GetNumberOfPointsInCell(i),
                     "incorrect result");
  }

  std::cout << "\tTesting CellToPoint\n";
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");

  vtkm::Id expected1[] = { 1, 2, 2, 1, 2, 4, 4, 2, 2, 1, 2 };
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == expected1[i], "incorrect result");
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "Testing Case 2 (some points are not part of any cell): \n";
  cellset = MakeTestCellSet2();

  std::cout << "\tTesting PointToCell\n";
  vtkm::worklet::DispatcherMapTopology<WorkletPointToCell>().Invoke(cellset, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfCells(),
                   "result length not equal to number of cells");
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == cellset.GetNumberOfPointsInCell(i),
                     "incorrect result");
  }

  std::cout << "\tTesting CellToPoint\n";
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");

  vtkm::Id expected2[] = { 0, 1, 1, 0, 0, 2, 2, 0, 2, 0, 1 };
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(
      result.GetPortalConstControl().Get(i) == expected2[i], "incorrect result at ", i);
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "General Testing: \n";

  std::cout << "\tTesting resource releasing in CellSetExplicit\n";
  cellset.ReleaseResourcesExecution();
  VTKM_TEST_ASSERT(cellset.GetNumberOfCells() == ArrayLength(g_shapes) / 2,
                   "release execution resources should not change the number of cells");
  VTKM_TEST_ASSERT(cellset.GetNumberOfPoints() == ArrayLength(expected2),
                   "release execution resources should not change the number of points");

  std::cout << "\tTesting CellToPoint table caching\n";
  cellset = MakeTestCellSet2();
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(cellset.HasConnectivity(CellTag{}, PointTag{})),
                   "PointToCell table missing.");
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(!cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table exists before PrepareForInput.");

  // Test a raw PrepareForInput call:
  cellset.PrepareForInput(vtkm::cont::DeviceAdapterTagSerial{}, PointTag{}, CellTag{});

  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table missing after PrepareForInput.");

  cellset.ResetConnectivity(PointTag{}, CellTag{});
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(!cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table exists after resetting.");

  // Test a PrepareForInput wrapped inside a dispatch (See #268)
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "CellToPoint table missing after CellToPoint worklet exec.");
}

} // anonymous namespace

int UnitTestCellSetExplicit(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetExplicit, argc, argv);
}
