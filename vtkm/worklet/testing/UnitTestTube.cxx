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

void TestTubeWorklets()
{
  std::cout << "Testing Tube Worklet" << std::endl;
  vtkm::cont::DataSetBuilderExplicitIterative dsb;

  std::vector<vtkm::Id> ids;
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault, 3>(2, 0, 0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);




#if 0
  /*
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(3,1,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(4,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(5,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(6,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(7,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(8,0,0), ids);
  */
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  std::cout<<"PolyLine: ids= "<<ids<<std::endl;
  ids.clear();

  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_LINE, ids);
  ids.clear();


  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(2,0,0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  ids.clear();


  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(0,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(1,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(2,0,0), ids);
  appendPts(dsb, vtkm::Vec<vtkm::FloatDefault,3>(3,0,0), ids);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);
  ids.clear();
#endif

#if 0
  //0-5
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,0,0));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(1,0,0));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(2,1,0));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(3,0,0));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(4,1,0));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(5,0,0));

  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, {0,1,2,3,4,5});

  //6-7
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,0,2));
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, {6,7});

  //8-9
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,1,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,1,2));
  dsb.AddCell(vtkm::CELL_SHAPE_LINE, {8,9});

  //10-12
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(1,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(1,1,1));
  dsb.AddCell(vtkm::CELL_SHAPE_TRIANGLE, {10,11,12});

  //13-20
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(0,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(1,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(2,1,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(3,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(4,1,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(5,0,1));
  dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(6,1,1));
  int x = dsb.AddPoint(vtkm::Vec<vtkm::FloatDefault,3>(7,0,1));
  std::cout<<"X= "<<x<<std::endl;
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, {13,14,15,16,17,18,19,20});
#endif

  vtkm::cont::DataSet ds = dsb.Create();
  ds.PrintSummary(std::cout);

  vtkm::cont::CoordinateSystem pts;
  vtkm::cont::CellSetSingleType<> cells("polyLines");

  vtkm::worklet::Tube tubeWorklet(2, 0.1f);
  tubeWorklet.Run(ds.GetCoordinateSystem(0), ds.GetCellSet(0));
}
}

int UnitTestTube(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTubeWorklets, argc, argv);
}
