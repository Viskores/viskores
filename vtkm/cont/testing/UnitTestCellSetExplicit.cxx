//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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

vtkm::UInt8 g_shapes[] = { static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_HEXAHEDRON),
                           static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_PYRAMID),
                           static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_TETRA),
                           static_cast<vtkm::UInt8>(vtkm::CELL_SHAPE_WEDGE) };

vtkm::IdComponent g_numIndices[] = { 8, 5, 4, 6 };

vtkm::Id g_indexOffset[] = { 0, 8, 13, 17 };

vtkm::Id g_connectivity[] = {
  0, 1, 5, 4, 3, 2, 6, 7, 1, 5, 6, 2, 8, 5, 8, 10, 6, 4, 7, 9, 5, 6, 10
};

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
          vtkm::cont::make_ArrayHandle(g_shapes, 4),
          vtkm::cont::make_ArrayHandle(g_numIndices, 4),
          vtkm::cont::make_ArrayHandle(g_connectivity, ArrayLength(g_connectivity)),
          vtkm::cont::make_ArrayHandle(g_indexOffset, 4));
  return cs;
}

// some points are not part of any cell
vtkm::cont::CellSetExplicit<> MakeTestCellSet2()
{
  vtkm::cont::CellSetExplicit<> cs;
  cs.Fill(numberOfPoints,
          vtkm::cont::make_ArrayHandle(g_shapes + 1, 2),
          vtkm::cont::make_ArrayHandle(g_numIndices + 1, 2),
          vtkm::cont::make_ArrayHandle(g_connectivity + g_indexOffset[1],
                                       g_indexOffset[3] - g_indexOffset[1]));
  return cs;
}

struct WorkletPointToCell : public vtkm::worklet::WorkletMapPointToCell
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell<IdType> numPoints);
  using ExecutionSignature = void(PointIndices, _2);
  using InputDomain = _1;

  template <typename PointIndicesType>
  VTKM_EXEC void operator()(const PointIndicesType& pointIndices, vtkm::Id& numPoints) const
  {
    numPoints = pointIndices.GetNumberOfComponents();
  }
};

struct WorkletCellToPoint : public vtkm::worklet::WorkletMapCellToPoint
{
  using ControlSignature = void(CellSetIn cellset, FieldOutPoint<IdType> numCells);
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
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == g_numIndices[i], "incorrect result");
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
  VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(0) == g_numIndices[1] &&
                     result.GetPortalConstControl().Get(1) == g_numIndices[2],
                   "incorrect result");

  std::cout << "\tTesting CellToPoint\n";
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == cellset.GetNumberOfPoints(),
                   "result length not equal to number of points");

  vtkm::Id expected2[] = { 0, 1, 1, 0, 0, 2, 2, 0, 2, 0, 1 };
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) == expected2[i], "incorrect result");
  }

  std::cout << "----------------------------------------------------\n";
  std::cout << "General Testing: \n";

  std::cout << "\tTesting resource releasing in CellSetExplicit\n";
  cellset.ReleaseResourcesExecution();
  VTKM_TEST_ASSERT(cellset.GetNumberOfCells() == ArrayLength(g_numIndices) / 2,
                   "release execution resources should not change the number of cells");
  VTKM_TEST_ASSERT(cellset.GetNumberOfPoints() == ArrayLength(expected2),
                   "release execution resources should not change the number of points");

  std::cout << "\tTesting CellToPoint table caching\n";
  cellset = MakeTestCellSet2();
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(cellset.HasConnectivity(PointTag{}, CellTag{})),
                   "PointToCell table missing.");
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(!cellset.HasConnectivity(CellTag{}, PointTag{})),
                   "CellToPoint table exists before PrepareForInput.");

  // Test a raw PrepareForInput call:
  cellset.PrepareForInput(VTKM_DEFAULT_DEVICE_ADAPTER_TAG{}, CellTag{}, PointTag{});

  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(cellset.HasConnectivity(CellTag{}, PointTag{})),
                   "CellToPoint table missing after PrepareForInput.");

  cellset.ResetConnectivity(CellTag{}, PointTag{});
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(!cellset.HasConnectivity(CellTag{}, PointTag{})),
                   "CellToPoint table exists after resetting.");

  // Test a PrepareForInput wrapped inside a dispatch (See #268)
  vtkm::worklet::DispatcherMapTopology<WorkletCellToPoint>().Invoke(cellset, result);
  VTKM_TEST_ASSERT(VTKM_PASS_COMMAS(cellset.HasConnectivity(CellTag{}, PointTag{})),
                   "CellToPoint table missing after CellToPoint worklet exec.");
}

} // anonymous namespace

int UnitTestCellSetExplicit(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetExplicit);
}
