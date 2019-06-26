//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/Tube.h>

namespace
{
void appendPts(vtkm::cont::DataSetBuilderExplicitIterative& dsb,
               const vtkm::Vec<vtkm::FloatDefault, 3>& pt,
               std::vector<vtkm::Id>& ids)
{
  vtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void createNonPoly(vtkm::cont::DataSetBuilderExplicitIterative& dsb)

{
  std::vector<vtkm::Id> ids;

  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 1, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_TRIANGLE, ids);
}

void TestTube(bool capEnds, vtkm::FloatDefault radius, vtkm::Id numSides, vtkm::Id insertNonPolyPos)
{
  using VecType = vtkm::Vec<vtkm::FloatDefault, 3>;

  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  if (insertNonPolyPos == 0)
    createNonPoly(dsb);

  vtkm::Id reqNumPts = 0, reqNumCells = 0;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += (ids.size() * numSides + (capEnds ? 2 : 0));
  reqNumCells += (2 * (ids.size() - 1) * numSides + (capEnds ? 2 * numSides : 0));

  if (insertNonPolyPos == 1)
    createNonPoly(dsb);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += (ids.size() * numSides + (capEnds ? 2 : 0));
  reqNumCells += (2 * (ids.size() - 1) * numSides + (capEnds ? 2 * numSides : 0));

  if (insertNonPolyPos == 2)
    createNonPoly(dsb);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  appendPts(dsb, VecType(4, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += (ids.size() * numSides + (capEnds ? 2 : 0));
  reqNumCells += (2 * (ids.size() - 1) * numSides + (capEnds ? 2 * numSides : 0));

  if (insertNonPolyPos == 3)
    createNonPoly(dsb);

  //Add something a little more complicated...
  ids.clear();
  vtkm::FloatDefault x0 = 0;
  vtkm::FloatDefault x1 = static_cast<vtkm::FloatDefault>(6.28);
  vtkm::FloatDefault dx = static_cast<vtkm::FloatDefault>(0.05);
  for (vtkm::FloatDefault x = x0; x < x1; x += dx)
    appendPts(dsb, VecType(x, vtkm::Cos(x), vtkm::Sin(x) / 2), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += (ids.size() * numSides + (capEnds ? 2 : 0));
  reqNumCells += (2 * (ids.size() - 1) * numSides + (capEnds ? 2 * numSides : 0));

  if (insertNonPolyPos == 4)
    createNonPoly(dsb);

  vtkm::cont::DataSet ds = dsb.Create();

  vtkm::worklet::Tube tubeWorklet(capEnds, numSides, radius);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  tubeWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(0), newPoints, newCells);

  VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == reqNumPts,
                   "Wrong number of points in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == reqNumCells, "Wrong cell shape in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE,
                   "Wrong cell shape in Tube worklet");
}

void TestTubeWorklets()
{
  std::cout << "Testing Tube Worklet" << std::endl;

  std::vector<vtkm::Id> testNumSides = { 3, 4, 8, 13, 20 };
  std::vector<vtkm::FloatDefault> testRadii = { 0.01f, 0.05f, 0.10f };
  std::vector<int> insertNonPolylinePos = { -1, 0, 1, 2, 3, 4 };

  for (auto& i : insertNonPolylinePos)
    for (auto& n : testNumSides)
      for (auto& r : testRadii)
      {
        TestTube(false, r, n, i);
        TestTube(true, r, n, i);
      }
}
}

int UnitTestTube(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTubeWorklets, argc, argv);
}
