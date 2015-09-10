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

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

vtkm::cont::DataSet MakeTetrahedralizeTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  float mins[3] = {-1.0f, -1.0f, -1.0f};
  float maxs[3] = {1.0f, 1.0f, 1.0f};

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
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
// Create a uniform structured cell set as input
// Add a field which is the index type which is (i+j+k) % 2 to alternate tetrahedralization pattern
// Create an unstructured cell set explicit as output
// Points are all the same, but each hexahedron cell becomes 5 tetrahedral cells
//
void TestTetrahedralizeUniformGrid()
{
  std::cout << "Testing TetrahedralizeUniformGrid Filter" << std::endl;
  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

  // Create the input uniform cell set, populated with index type
  vtkm::Id3 dims(4,4,4);
  vtkm::cont::DataSet dataSet = MakeTetrahedralizeTestDataSet(dims);

  // Set the number of points supporting the cell set
  const vtkm::Id nVerts = (dims[0] + 1) * (dims[1] + 1) * (dims[2] + 1); 

  // Create the explicit cell set without the cells which is empty
  vtkm::cont::CellSetExplicit<> cellSet(nVerts, "cells", 3);

  // Get the points from the input dataset and copy to the output dataset (PKF)

  vtkm::worklet::TetrahedralizeFilterUniformGrid<DeviceAdapter> tetrahedralizeFilter(dims, dataSet, cellSet);
  tetrahedralizeFilter.Run();

  // Five tets are created for every hex cell
  VTKM_TEST_ASSERT(test_equal(cellSet.GetNumberOfCells(), 64 * 5),
                   "Wrong result for Tetrahedralize filter");
}

int UnitTestTetrahedralizeUniformGrid(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestTetrahedralizeUniformGrid);
}
