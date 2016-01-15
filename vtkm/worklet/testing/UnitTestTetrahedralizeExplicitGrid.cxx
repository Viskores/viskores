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
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coordinates;
  coordinates.push_back( CoordType(0, 0, 0) );   // 0
  coordinates.push_back( CoordType(1, 0, 0) );   // 1
  coordinates.push_back( CoordType(2, 0, 0) );   // 2
  coordinates.push_back( CoordType(3, 0, 0) );   // 3
  coordinates.push_back( CoordType(0, 1, 0) );   // 4
  coordinates.push_back( CoordType(1, 1, 0) );   // 5
  coordinates.push_back( CoordType(2, 1, 0) );   // 6
  coordinates.push_back( CoordType(3, 1, 0) );   // 7
  coordinates.push_back( CoordType(0, 2, 0) );   // 8
  coordinates.push_back( CoordType(1, 2, 0) );   // 9
  coordinates.push_back( CoordType(2, 2, 0) );   // 10
  coordinates.push_back( CoordType(3, 2, 0) );   // 11
  coordinates.push_back( CoordType(0, 3, 0) );   // 12
  coordinates.push_back( CoordType(3, 3, 0) );   // 13
  coordinates.push_back( CoordType(1, 4, 0) );   // 14
  coordinates.push_back( CoordType(2, 4, 0) );   // 15


  std::vector<vtkm::UInt8> shapes;
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);

  std::vector<vtkm::IdComponent> numindices;
  numindices.push_back(3);
  numindices.push_back(4);
  numindices.push_back(4);
  numindices.push_back(4);
  numindices.push_back(3);
  numindices.push_back(4);
  numindices.push_back(6);

  std::vector<vtkm::Id> conn;
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);

  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);

  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(10);
  conn.push_back(9);

  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(9);
  conn.push_back(8);

  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);

  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(11);
  conn.push_back(10);

  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(13);
  conn.push_back(15);
  conn.push_back(14);
  conn.push_back(12);

  vtkm::cont::DataSetBuilderExplicit builder;
  return builder.Create(coordinates, shapes, numindices, conn, 2);
}

//
// Test 3D explicit dataset
//
vtkm::cont::DataSet MakeTetrahedralizeExplicitDataSet()
{
  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coordinates;
  coordinates.push_back( CoordType(0, 0, 0) );
  coordinates.push_back( CoordType(1, 0, 0) );
  coordinates.push_back( CoordType(2, 0, 0) );
  coordinates.push_back( CoordType(3, 0, 0) );
  coordinates.push_back( CoordType(0, 1, 0) );
  coordinates.push_back( CoordType(1, 1, 0) );
  coordinates.push_back( CoordType(2, 1, 0) );
  coordinates.push_back( CoordType(2.5, 1, 0) );
  coordinates.push_back( CoordType(0, 2, 0) );
  coordinates.push_back( CoordType(1, 2, 0) );
  coordinates.push_back( CoordType(0.5, 0.5, 1) );
  coordinates.push_back( CoordType(1, 0, 1) );
  coordinates.push_back( CoordType(2, 0, 1) );
  coordinates.push_back( CoordType(3, 0, 1) );
  coordinates.push_back( CoordType(1, 1, 1) );
  coordinates.push_back( CoordType(2, 1, 1) );
  coordinates.push_back( CoordType(2.5, 1, 1) );
  coordinates.push_back( CoordType(0.5, 1.5, 1) );

  std::vector<vtkm::UInt8> shapes;
  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);

  std::vector<vtkm::IdComponent> numindices;
  numindices.push_back(4);
  numindices.push_back(8);
  numindices.push_back(6);
  numindices.push_back(5);

  std::vector<vtkm::Id> conn;
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(5);
  conn.push_back(10);

  conn.push_back(1);
  conn.push_back(2);
  conn.push_back(6);
  conn.push_back(5);
  conn.push_back(11);
  conn.push_back(12);
  conn.push_back(15);
  conn.push_back(14);

  conn.push_back(2);
  conn.push_back(3);
  conn.push_back(7);
  conn.push_back(12);
  conn.push_back(13);
  conn.push_back(16);

  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(9);
  conn.push_back(8);
  conn.push_back(17);

  vtkm::cont::DataSetBuilderExplicit builder;
  return builder.Create(coordinates, shapes, numindices, conn);

}

}

//
// Create an explicit 2D cell set as input and fill
// Create an explicit 2D cell set as output
// Points are all the same, but each cell becomes triangle cells
//
void TestExplicitGrid2D()
{
  std::cout << "Testing TriangulationExplicitGrid Filter" << std::endl;
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Create the input uniform cell set
  vtkm::cont::DataSet inDataSet = MakeTriangulateExplicitDataSet();

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::DataSet outDataSet;
  vtkm::cont::CellSetSingleType<> outCellSet(vtkm::CellShapeTagTriangle(), "cells");
  outDataSet.AddCellSet(outCellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert explicit cells to triangles
  vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter>
                 tetrahedralizeFilter(inDataSet, outDataSet);
  tetrahedralizeFilter.Run();

  vtkm::cont::CellSetSingleType<> cellSet = outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetSingleType<> >();
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray = coordinates.GetData();
  std::cout << "Number of output triangles " << cellSet.GetNumberOfCells() << std::endl;
  std::cout << "Number of output vertices " << coordArray.GetNumberOfValues() << std::endl;
  std::cout << "Number of output components " << coordArray.GetNumberOfComponents() << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Float64> bounds = coordinates.GetBounds(DeviceAdapter());
  std::cout << "Bounds ("
            << bounds.GetPortalControl().Get(0) << "," << bounds.GetPortalControl().Get(1) << ") ("
            << bounds.GetPortalControl().Get(2) << "," << bounds.GetPortalControl().Get(3) << ")" << std::endl;

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
  vtkm::cont::CellSetSingleType<> outCellSet(vtkm::CellShapeTagTetra(), "cells");
  outDataSet.AddCellSet(outCellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert explicit cells to triangles
  vtkm::worklet::TetrahedralizeFilterExplicitGrid<DeviceAdapter>
                 tetrahedralizeFilter(inDataSet, outDataSet);
  tetrahedralizeFilter.Run();

  vtkm::cont::CellSetSingleType<> cellSet = outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetSingleType<> >();
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem coordArray = coordinates.GetData();
  std::cout << "Number of output tetrahedra " << cellSet.GetNumberOfCells() << std::endl;
  std::cout << "Number of output vertices " << coordArray.GetNumberOfValues() << std::endl;
  std::cout << "Number of output components " << coordArray.GetNumberOfComponents() << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Float64> bounds = coordinates.GetBounds(DeviceAdapter());
  std::cout << "Bounds ("
            << bounds.GetPortalControl().Get(0) << "," << bounds.GetPortalControl().Get(1) << ") ("
            << bounds.GetPortalControl().Get(2) << "," << bounds.GetPortalControl().Get(3) << ") ("
            << bounds.GetPortalControl().Get(4) << "," << bounds.GetPortalControl().Get(5) << ")" << std::endl;

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
