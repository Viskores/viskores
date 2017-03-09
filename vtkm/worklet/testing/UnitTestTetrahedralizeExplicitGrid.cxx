//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/TetrahedralizeExplicitGrid.h>
#include <vtkm/worklet/TriangulateExplicitGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/Testing.h>

namespace {

//
// Test 2D explicit dataset
//
vtkm::cont::DataSet MakeTriangulateExplicitDataSet()
{
  vtkm::cont::DataSetBuilderExplicitIterative builder;
  builder.Begin();

  builder.AddPoint(0, 0, 0);   // 0
  builder.AddPoint(1, 0, 0);   // 1
  builder.AddPoint(2, 0, 0);   // 2
  builder.AddPoint(3, 0, 0);   // 3
  builder.AddPoint(0, 1, 0);   // 4
  builder.AddPoint(1, 1, 0);   // 5
  builder.AddPoint(2, 1, 0);   // 6
  builder.AddPoint(3, 1, 0);   // 7
  builder.AddPoint(0, 2, 0);   // 8
  builder.AddPoint(1, 2, 0);   // 9
  builder.AddPoint(2, 2, 0);   // 10
  builder.AddPoint(3, 2, 0);   // 11
  builder.AddPoint(0, 3, 0);   // 12
  builder.AddPoint(3, 3, 0);   // 13
  builder.AddPoint(1, 4, 0);   // 14
  builder.AddPoint(2, 4, 0);   // 15

  builder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  builder.AddCellPoint(0);
  builder.AddCellPoint(1);
  builder.AddCellPoint(5);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(1);
  builder.AddCellPoint(2);
  builder.AddCellPoint(6);
  builder.AddCellPoint(5);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(5);
  builder.AddCellPoint(6);
  builder.AddCellPoint(10);
  builder.AddCellPoint(9);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(4);
  builder.AddCellPoint(5);
  builder.AddCellPoint(9);
  builder.AddCellPoint(8);

  builder.AddCell(vtkm::CELL_SHAPE_TRIANGLE);
  builder.AddCellPoint(2);
  builder.AddCellPoint(3);
  builder.AddCellPoint(7);

  builder.AddCell(vtkm::CELL_SHAPE_QUAD);
  builder.AddCellPoint(6);
  builder.AddCellPoint(7);
  builder.AddCellPoint(11);
  builder.AddCellPoint(10);

  builder.AddCell(vtkm::CELL_SHAPE_POLYGON);
  builder.AddCellPoint(9);
  builder.AddCellPoint(10);
  builder.AddCellPoint(13);
  builder.AddCellPoint(15);
  builder.AddCellPoint(14);
  builder.AddCellPoint(12);

  return builder.Create();
}

//
// Test 3D explicit dataset
//
vtkm::cont::DataSet MakeTetrahedralizeExplicitDataSet()
{
  vtkm::cont::DataSetBuilderExplicitIterative builder;
  builder.Begin();

  builder.AddPoint(0, 0, 0);
  builder.AddPoint(1, 0, 0);
  builder.AddPoint(2, 0, 0);
  builder.AddPoint(3, 0, 0);
  builder.AddPoint(0, 1, 0);
  builder.AddPoint(1, 1, 0);
  builder.AddPoint(2, 1, 0);
  builder.AddPoint(2.5, 1.0, 0.0);
  builder.AddPoint(0, 2, 0);
  builder.AddPoint(1, 2, 0);
  builder.AddPoint(0.5, 0.5, 1.0);
  builder.AddPoint(1, 0, 1);
  builder.AddPoint(2, 0, 1);
  builder.AddPoint(3, 0, 1);
  builder.AddPoint(1, 1, 1);
  builder.AddPoint(2, 1, 1);
  builder.AddPoint(2.5, 1.0, 1.0);
  builder.AddPoint(0.5, 1.5, 1.0);

  builder.AddCell(vtkm::CELL_SHAPE_TETRA);
  builder.AddCellPoint(0);
  builder.AddCellPoint(1);
  builder.AddCellPoint(5);
  builder.AddCellPoint(10);

  builder.AddCell(vtkm::CELL_SHAPE_HEXAHEDRON);
  builder.AddCellPoint(1);
  builder.AddCellPoint(2);
  builder.AddCellPoint(6);
  builder.AddCellPoint(5);
  builder.AddCellPoint(11);
  builder.AddCellPoint(12);
  builder.AddCellPoint(15);
  builder.AddCellPoint(14);

  builder.AddCell(vtkm::CELL_SHAPE_WEDGE);
  builder.AddCellPoint(2);
  builder.AddCellPoint(3);
  builder.AddCellPoint(7);
  builder.AddCellPoint(12);
  builder.AddCellPoint(13);
  builder.AddCellPoint(16);

  builder.AddCell(vtkm::CELL_SHAPE_PYRAMID);
  builder.AddCellPoint(4);
  builder.AddCellPoint(5);
  builder.AddCellPoint(9);
  builder.AddCellPoint(8);
  builder.AddCellPoint(17);

  return builder.Create();

}

}

//
// Create an explicit 2D cell set as input and fill
// Create an explicit 2D cell set as output
// Points are all the same, but each cell becomes triangle cells
//
void TestExplicitGrid2D()
{
  std::cout << "Testing TriangulateExplicitGrid Filter" << std::endl;
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTriangulateExplicitDataSet();

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::DataSet outDataSet;
  vtkm::cont::CellSetSingleType<> outCellSet("cells");
  outDataSet.AddCellSet(outCellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert explicit cells to triangles
  vtkm::worklet::TriangulateFilterExplicitGrid<DeviceAdapter>
                 triangulateFilter(inDataSet, outDataSet);
  triangulateFilter.Run();

  vtkm::cont::CellSetSingleType<> cellSet;
  outDataSet.GetCellSet(0).CopyTo(cellSet);
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray = coordinates.GetData();
  std::cout << "Number of output triangles " << cellSet.GetNumberOfCells() << std::endl;
  std::cout << "Number of output vertices " << coordArray.GetNumberOfValues() << std::endl;
  std::cout << "Number of output components " << coordArray.GetNumberOfComponents() << std::endl;

  vtkm::Bounds bounds = coordinates.GetBounds();
  std::cout << "Bounds " << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 14), "Wrong result for Triangulate filter");
}

//
// Create an explicit 3D cell set as input and fill
// Create an explicit 3D cell set as output
// Points are all the same, but each cell becomes tetrahedra
//
void TestExplicitGrid3D()
{
  std::cout << "Testing TetrahedralizeExplicitGrid Filter" << std::endl;
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTetrahedralizeExplicitDataSet();

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::DataSet outDataSet;
  vtkm::cont::CellSetSingleType<> outCellSet("cells");
  outDataSet.AddCellSet(outCellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert explicit cells to triangles
  vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter>
                 tetrahedralizeFilter(inDataSet, outDataSet);
  tetrahedralizeFilter.Run();

  vtkm::cont::CellSetSingleType<> cellSet;
  outDataSet.GetCellSet(0).CopyTo(cellSet);
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray = coordinates.GetData();
  std::cout << "Number of output tetrahedra " << cellSet.GetNumberOfCells() << std::endl;
  std::cout << "Number of output vertices " << coordArray.GetNumberOfValues() << std::endl;
  std::cout << "Number of output components " << coordArray.GetNumberOfComponents() << std::endl;

  vtkm::Bounds bounds = coordinates.GetBounds();
  std::cout << "Bounds " << bounds << std::endl;

  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 11), "Wrong result for Tetrahedralize filter");
}

void TestTetrahedralizeExplicitGrid()
{
  TestExplicitGrid2D();
  TestExplicitGrid3D();
}

int UnitTestTetrahedralizeExplicitGrid(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestTetrahedralizeExplicitGrid);
}
