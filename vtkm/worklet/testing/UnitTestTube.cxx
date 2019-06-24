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
template <class T>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T>& v)
{
  typename std::vector<T>::const_iterator b = v.begin();
  typename std::vector<T>::const_iterator e = v.end();

  out << "[";
  while (b != e)
  {
    out << *b;
    std::advance(b, 1);
    if (b != e)
      out << " ";
  }
  out << "]";
  return out;
}

void appendPts(vtkm::cont::DataSetBuilderExplicitIterative& dsb,
               const vtkm::Vec<vtkm::FloatDefault, 3>& pt,
               std::vector<vtkm::Id>& ids)
{
  vtkm::Id pid = dsb.AddPoint(pt);
  ids.push_back(pid);
}
#if 1
void TestTubeWorkletsDebug()
{
  std::cout << "Testing Tube Worklet" << std::endl;
  vtkm::cont::DataSetBuilderExplicitIterative dsb;

  std::vector<vtkm::Id> ids;

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

#if 0
  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,4), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,3), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_LINE, ids);

  ids.clear();
  vtkm::FloatDefault x0 = 0, x1 = 6.28, dx = 0.05;
  //x1 = 6.28 / 6.;
  for (vtkm::FloatDefault x = x0; x < x1; x += dx)
      appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(x,vtkm::Cos(x),.5*vtkm::Sin(x)), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);



  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,4), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,3), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);


  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,4), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,3), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_LINE, ids);

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,4), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,3), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_LINE, ids);


  ids.clear();
  for (vtkm::FloatDefault x = x0; x < x1; x += dx)
      appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(x,2+vtkm::Cos(x),.5*vtkm::Sin(x)), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
#endif

  vtkm::cont::DataSet ds = dsb.Create();
  ds.PrintSummary(std::cout);

  vtkm::worklet::Tube tubeWorklet(true, 17, 0.05f);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  tubeWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(0), newPoints, newCells);

  vtkm::cont::DataSet ds2;
  ds2.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", newPoints));
  ds2.AddCellSet(newCells);
  ds2.PrintSummary(std::cout);

  vtkm::io::writer::VTKDataSetWriter writer("tube.vtk");
  writer.WriteDataSet(ds2);

  if (1)
  {
    vtkm::io::writer::VTKDataSetWriter writer("poly.vtk");
    writer.WriteDataSet(ds);
  }

  if (1)
  {
    dsb = vtkm::cont::DataSetBuilderExplicitIterative();
    int nPts = newPoints.GetNumberOfValues();
    ids.clear();
    auto portal = newPoints.GetPortalControl();
    for (int i = 0; i < nPts; i++)
    {
      vtkm::Id id = dsb.AddPoint(portal.Get(i));
      dsb.AddCell(vtkm::CELL_SHAPE_VERTEX, { id });
    }
    ds = dsb.Create();
    vtkm::io::writer::VTKDataSetWriter writer("pts.vtk");
    writer.WriteDataSet(ds);
  }
}
#endif

// Test with 3 polylines
void TestTube1()
{
  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(2, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(2, 1, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(3, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(4, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  vtkm::cont::DataSet ds = dsb.Create();

  vtkm::worklet::Tube tubeWorklet(true, 13, 0.05f);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  tubeWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(0), newPoints, newCells);

  VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == 130, "Wrong number of points in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE,
                   "Wrong cell shape in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == 234, "Wrong cell shape in Tube worklet");
}

// Test with 2 polylines and 1 triangle (which should be skipped).
void TestTube2()
{
  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 1, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_TRIANGLE, ids);

  ids.clear();
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(2, 1, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(3, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(4, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  vtkm::cont::DataSet ds = dsb.Create();

  vtkm::worklet::Tube tubeWorklet(true, 13, 0.05f);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> newPoints;
  vtkm::cont::CellSetSingleType<> newCells;
  tubeWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(0), newPoints, newCells);

  VTKM_TEST_ASSERT(newPoints.GetNumberOfValues() == 91, "Wrong number of points in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE,
                   "Wrong cell shape in Tube worklet");
  VTKM_TEST_ASSERT(newCells.GetNumberOfCells() == 156, "Wrong cell shape in Tube worklet");
}

void TestTubeWorklets()
{
  std::cout << "Testing Tube Worklet" << std::endl;
  TestTubeWorkletsDebug();
  //TestTube1();
  //TestTube2();
}
}

int UnitTestTube(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTubeWorklets, argc, argv);
}
