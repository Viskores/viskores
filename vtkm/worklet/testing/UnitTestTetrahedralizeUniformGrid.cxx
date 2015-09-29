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

#include <vtkm/worklet/TetrahedralizeUniformGrid.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace {

//
// Test 2D regular dataset
//
vtkm::cont::DataSet MakeTriangulateTestDataSet(vtkm::Id2 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, 1);
  const vtkm::Vec<vtkm::Float32, 3> origin = vtkm::make_Vec(0.0f, 0.0f, 0.0f);
  const vtkm::Vec<vtkm::Float32, 3> spacing = vtkm::make_Vec(
                                              1.0f/static_cast<vtkm::Float32>(dims[0]),
                                              1.0f/static_cast<vtkm::Float32>(dims[1]),
                                              1.0f/static_cast<vtkm::Float32>(dims[2]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

  static const vtkm::IdComponent ndim = 2;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vtkm::make_Vec(dims[0] + 1, dims[1] + 1));
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

//
// Test 3D regular dataset
//
vtkm::cont::DataSet MakeTetrahedralizeTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);
  const vtkm::Vec<vtkm::Float32, 3> origin = vtkm::make_Vec(0.0f, 0.0f, 0.0f);
  const vtkm::Vec<vtkm::Float32, 3> spacing = vtkm::make_Vec(
                                              1.0f/static_cast<vtkm::Float32>(dims[0]),
                                              1.0f/static_cast<vtkm::Float32>(dims[1]),
                                              1.0f/static_cast<vtkm::Float32>(dims[2]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(
          vtkm::cont::CoordinateSystem("coordinates", 1, coordinates));

  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

}

//
// Create a uniform 2D structured cell set as input
// Add a field which is the index type which is (i+j) % 2 to alternate triangulation pattern
// Create an unstructured cell set explicit as output
// Points are all the same, but each quad cell becomes 2 triangle cells
//
void TestUniformGrid2D()
{
  std::cout << "Testing TriangulationUniformGrid Filter" << std::endl;
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Create the input uniform cell set
  vtkm::Id2 dims(4,4);
  vtkm::cont::DataSet inDataSet = MakeTriangulateTestDataSet(dims);

  // Set number of cells and vertices in input dataset
  vtkm::Id numberOfCells = dims[0] * dims[1];
  vtkm::Id numberOfVertices = (dims[0] + 1) * (dims[1] + 1);
  std::cout << "Number of input quads " << numberOfCells << std::endl;
  std::cout << "Number of input vertices " << numberOfVertices << std::endl;

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::DataSet outDataSet;
  vtkm::cont::CellSetSingleType<> outCellSet(vtkm::CellShapeTagTriangle(), "cells");
  outDataSet.AddCellSet(outCellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert uniform hexahedra to tetrahedra
  vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter> 
                 tetrahedralizeFilter(inDataSet, outDataSet);
  tetrahedralizeFilter.Run();

  vtkm::cont::CellSetSingleType<> &cellSet = outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetSingleType<> >();
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem &coordArray = coordinates.GetData();
  std::cout << "Number of output triangles " << cellSet.GetNumberOfCells() << std::endl;
  std::cout << "Number of output vertices " << coordArray.GetNumberOfValues() << std::endl;
  std::cout << "Number of output components " << coordArray.GetNumberOfComponents() << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Float64> bounds = coordinates.GetBounds(DeviceAdapter());
  std::cout << "Bounds (" 
            << bounds.GetPortalControl().Get(0) << "," << bounds.GetPortalControl().Get(1) << ") ("
            << bounds.GetPortalControl().Get(2) << "," << bounds.GetPortalControl().Get(3) << ") ("
            << bounds.GetPortalControl().Get(4) << "," << bounds.GetPortalControl().Get(5) << ")" << std::endl;

  // Two triangles are created for every quad cell
  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), numberOfCells * 2),
                   "Wrong result for Triangulate filter");
}

//
// Create a uniform 3D structured cell set as input
// Add a field which is the index type which is (i+j+k) % 2 to alternate tetrahedralization pattern
// Create an unstructured cell set explicit as output
// Points are all the same, but each hexahedron cell becomes 5 tetrahedral cells
//
void TestUniformGrid3D()
{
  std::cout << "Testing TetrahedralizeUniformGrid Filter" << std::endl;
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Create the input uniform cell set
  vtkm::Id3 dims(4,4,4);
  vtkm::cont::DataSet inDataSet = MakeTetrahedralizeTestDataSet(dims);

  // Set number of cells and vertices in input dataset
  vtkm::Id numberOfCells = dims[0] * dims[1] * dims[2];
  vtkm::Id numberOfVertices = (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1);
  std::cout << "Number of input hexahedra " << numberOfCells << std::endl;
  std::cout << "Number of input vertices " << numberOfVertices << std::endl;

  // Create the output dataset explicit cell set with same coordinate system
  vtkm::cont::DataSet outDataSet;
  vtkm::cont::CellSetSingleType<> outCellSet(vtkm::CellShapeTagTetra(), "cells");
  outDataSet.AddCellSet(outCellSet);
  outDataSet.AddCoordinateSystem(inDataSet.GetCoordinateSystem(0));

  // Convert uniform hexahedra to tetrahedra
  vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter> 
                 tetrahedralizeFilter(inDataSet, outDataSet);
  tetrahedralizeFilter.Run();

  vtkm::cont::CellSetSingleType<> &cellSet = outDataSet.GetCellSet(0).CastTo<vtkm::cont::CellSetSingleType<> >();
  vtkm::cont::CoordinateSystem coordinates = outDataSet.GetCoordinateSystem(0);
  const vtkm::cont::DynamicArrayHandleCoordinateSystem &coordArray = coordinates.GetData();
  std::cout << "Number of output tetrahedra " << cellSet.GetNumberOfCells() << std::endl;
  std::cout << "Number of output vertices " << coordArray.GetNumberOfValues() << std::endl;
  std::cout << "Number of output components " << coordArray.GetNumberOfComponents() << std::endl;

  vtkm::cont::ArrayHandle<vtkm::Float64> bounds = coordinates.GetBounds(DeviceAdapter());
  std::cout << "Bounds (" 
            << bounds.GetPortalControl().Get(0) << "," << bounds.GetPortalControl().Get(1) << ") ("
            << bounds.GetPortalControl().Get(2) << "," << bounds.GetPortalControl().Get(3) << ") ("
            << bounds.GetPortalControl().Get(4) << "," << bounds.GetPortalControl().Get(5) << ")" << std::endl;

  // Five tets are created for every hex cell
  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), numberOfCells * 5),
                   "Wrong result for Tetrahedralize filter");
}

void TestTetrahedralizeUniformGrid()
{
  TestUniformGrid3D();
  TestUniformGrid2D();
}

int UnitTestTetrahedralizeUniformGrid(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestTetrahedralizeUniformGrid);
}
