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
#include <vtkm/worklet/StreamSurface.h>

namespace
{
void appendPts(vtkm::cont::DataSetBuilderExplicitIterative& dsb,
               const vtkm::Vec3f& pt,
               std::vector<vtkm::Id>& ids)
{
  vtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}

void TestSameNumPolylines()
{
  using VecType = vtkm::Vec3f;

  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 1), ids);
  appendPts(dsb, VecType(1, 1, 1), ids);
  appendPts(dsb, VecType(2, 1, 1), ids);
  appendPts(dsb, VecType(3, 0, 1), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 2), ids);
  appendPts(dsb, VecType(1, 1, 2), ids);
  appendPts(dsb, VecType(2, 1, 2), ids);
  appendPts(dsb, VecType(3, 0, 2), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  vtkm::cont::DataSet ds = dsb.Create();
  vtkm::worklet::StreamSurface streamSurfaceWorklet;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  streamSurfaceWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(), newPoints, newCells);

  VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == ds.GetCoordinateSystem(0).GetNumberOfValues(),
                   "Wrong number of points in StreamSurface worklet");
  VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == 12,
                   "Wrong number of cells in StreamSurface worklet");
  /*
  vtkm::cont::DataSet ds2;
  ds2.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", newPoints));
  ds2.SetCellSet(newCells);
  vtkm::io::writer::VTKDataSetWriter writer("srf.vtk");
  writer.WriteDataSet(ds2);
*/
}

void TestUnequalNumPolylines(int unequalType)
{
  using VecType = vtkm::Vec3f;

  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  ids.clear();
  appendPts(dsb, VecType(0, 0, 0), ids);
  appendPts(dsb, VecType(1, 1, 0), ids);
  appendPts(dsb, VecType(2, 1, 0), ids);
  appendPts(dsb, VecType(3, 0, 0), ids);
  if (unequalType == 0)
  {
    appendPts(dsb, VecType(4, 0, 0), ids);
    appendPts(dsb, VecType(5, 0, 0), ids);
    appendPts(dsb, VecType(6, 0, 0), ids);
  }
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 1), ids);
  appendPts(dsb, VecType(1, 1, 1), ids);
  appendPts(dsb, VecType(2, 1, 1), ids);
  appendPts(dsb, VecType(3, 0, 1), ids);
  if (unequalType == 1)
  {
    appendPts(dsb, VecType(4, 0, 1), ids);
    appendPts(dsb, VecType(5, 0, 1), ids);
    appendPts(dsb, VecType(6, 0, 1), ids);
  }
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, VecType(0, 0, 2), ids);
  appendPts(dsb, VecType(1, 1, 2), ids);
  appendPts(dsb, VecType(2, 1, 2), ids);
  appendPts(dsb, VecType(3, 0, 2), ids);
  if (unequalType == 2)
  {
    appendPts(dsb, VecType(4, 0, 2), ids);
    appendPts(dsb, VecType(5, 0, 2), ids);
    appendPts(dsb, VecType(6, 0, 2), ids);
  }
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  vtkm::cont::DataSet ds = dsb.Create();
  vtkm::worklet::StreamSurface streamSurfaceWorklet;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  streamSurfaceWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(), newPoints, newCells);

  vtkm::Id numRequiredCells = (unequalType == 1 ? 18 : 15);

  VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == ds.GetCoordinateSystem(0).GetNumberOfValues(),
                   "Wrong number of points in StreamSurface worklet");
  VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == numRequiredCells,
                   "Wrong number of cells in StreamSurface worklet");

  /*
  vtkm::cont::DataSet ds2;
  ds2.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", newPoints));
  ds2.SetCellSet(newCells);
  vtkm::io::writer::VTKDataSetWriter writer("srf.vtk");
  writer.WriteDataSet(ds2);
*/
}

void TestStreamSurface()
{
  std::cout << "Testing Stream Surface Worklet" << std::endl;
  TestSameNumPolylines();
  TestUnequalNumPolylines(0);
  TestUnequalNumPolylines(1);
  TestUnequalNumPolylines(2);
}
}

int UnitTestStreamSurface(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamSurface, argc, argv);
}
