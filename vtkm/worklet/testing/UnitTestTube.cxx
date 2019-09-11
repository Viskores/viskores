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
               const vtkm::Vec3f& pt,
               std::vector<vtkm::Id>& ids)
{
  vtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void createNonPoly(vtkm::cont::DataSetBuilderExplicitIterative& dsb)

{
  std::vector<vtkm::Id> ids;

  appendPts(dsb, vtkm::Vec3f(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec3f(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec3f(1, 1, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_TRIANGLE, ids);
}

vtkm::Id calcNumPoints(const std::size_t& numPtIds, const vtkm::Id& numSides, const bool& capEnds)
{
  //there are 'numSides' points for each polyline vertex
  //plus, 2 more for the center point of start and end caps.
  return static_cast<vtkm::Id>(numPtIds) * numSides + (capEnds ? 2 : 0);
}

vtkm::Id calcNumCells(const std::size_t& numPtIds, const vtkm::Id& numSides, const bool& capEnds)
{
  //Each line segment has numSides * 2 triangles.
  //plus, numSides triangles for each cap.
  return (2 * static_cast<vtkm::Id>(numPtIds - 1) * numSides) + (capEnds ? 2 * numSides : 0);
}

void TestTube(bool capEnds, vtkm::FloatDefault radius, vtkm::Id numSides, vtkm::Id insertNonPolyPos)
{
  using VecType = vtkm::Vec3f;

  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  if (insertNonPolyPos == 0)
    createNonPoly(dsb);

  vtkm::Id reqNumPts = 0, reqNumCells = 0;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 1)
    createNonPoly(dsb);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 2)
    createNonPoly(dsb);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 0, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  appendPts(dsb, VecType(4, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

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
  reqNumPts += calcNumPoints(ids.size(), numSides, capEnds);
  reqNumCells += calcNumCells(ids.size(), numSides, capEnds);

  if (insertNonPolyPos == 4)
    createNonPoly(dsb);

  //Finally, add a degenerate polyline: don't dance with the beast.
  ids.clear();
  appendPts(dsb, VecType(6, 6, 6), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  //Should NOT produce a tubed polyline, so don't increment reqNumPts and reqNumCells.

  vtkm::cont::DataSet ds = dsb.Create();

  vtkm::worklet::Tube tubeWorklet(capEnds, numSides, radius);
  vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  tubeWorklet.Run(ds.GetCoordinateSystem(0).GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Vec3f>>(),
                  ds.GetCellSet(),
                  newPoints,
                  newCells);

  VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == reqNumPts,
                   "Wrong number of points in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == reqNumCells, "Wrong cell shape in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE,
                   "Wrong cell shape in Tube worklet");
}

void TestLinearPolylines()
{
  using VecType = vtkm::Vec3f;

  //Create a number of linear polylines along a set of directions.
  //We check that the tubes are all copacetic (proper number of cells, points),
  //and that the tube points all lie in the proper plane.
  //This will validate the code that computes the coordinate frame at each
  //vertex in the polyline. There are numeric checks to handle co-linear segments.

  std::vector<VecType> dirs;
  for (int i = -1; i <= 1; i++)
    for (int j = -1; j <= 1; j++)
      for (int k = -1; k <= 1; k++)
      {
        if (!i && !j && !k)
          continue;
        dirs.push_back(vtkm::Normal(VecType(static_cast<vtkm::FloatDefault>(i),
                                            static_cast<vtkm::FloatDefault>(j),
                                            static_cast<vtkm::FloatDefault>(k))));
      }

  bool capEnds = false;
  vtkm::Id numSides = 3;
  vtkm::FloatDefault radius = 1;
  for (auto& dir : dirs)
  {
    vtkm::cont::DataSetBuilderExplicitIterative dsb;
    std::vector<vtkm::Id> ids;

    VecType pt(0, 0, 0);
    for (int i = 0; i < 5; i++)
    {
      appendPts(dsb, pt, ids);
      pt = pt + dir;
    }

    dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
    vtkm::cont::DataSet ds = dsb.Create();

    vtkm::Id reqNumPts = calcNumPoints(ids.size(), numSides, capEnds);
    vtkm::Id reqNumCells = calcNumCells(ids.size(), numSides, capEnds);

    vtkm::worklet::Tube tubeWorklet(capEnds, numSides, radius);
    vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
    vtkm::cont::CellSetSingleType<> newCells;
    tubeWorklet.Run(
      ds.GetCoordinateSystem(0).GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Vec3f>>(),
      ds.GetCellSet(),
      newPoints,
      newCells);

    VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == reqNumPts,
                     "Wrong number of points in Tube worklet");
    VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == reqNumCells,
                     "Wrong cell shape in Tube worklet");
    VTKM_TEST_ASSERT(newCells.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE,
                     "Wrong cell shape in Tube worklet");

    //Each of the 3 points should be in the plane defined by dir.
    auto portal = newPoints.GetPortalConstControl();
    for (vtkm::Id i = 0; i < newPoints.GetNumberOfValues(); i += 3)
    {
      auto p0 = portal.Get(i + 0);
      auto p1 = portal.Get(i + 1);
      auto p2 = portal.Get(i + 2);
      auto vec = vtkm::Normal(vtkm::Cross(p0 - p1, p0 - p2));
      vtkm::FloatDefault dp = vtkm::Abs(vtkm::Dot(vec, dir));
      VTKM_TEST_ASSERT((1 - dp) <= vtkm::Epsilon<vtkm::FloatDefault>(),
                       "Tube points in wrong orientation");
    }
  }
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

  TestLinearPolylines();
}
}

int UnitTestTube(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTubeWorklets, argc, argv);
}
