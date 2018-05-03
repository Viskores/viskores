//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/RemoveUnusedPoints.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

vtkm::cont::CellSetExplicit<> CreateInputCellSet()
{
  vtkm::cont::CellSetExplicit<> cellSet("cells");
  cellSet.PrepareToAddCells(2, 7);
  cellSet.AddCell(vtkm::CELL_SHAPE_TRIANGLE, 3, vtkm::make_Vec<vtkm::Id>(0, 2, 4));
  cellSet.AddCell(vtkm::CELL_SHAPE_QUAD, 4, vtkm::make_Vec<vtkm::Id>(4, 2, 6, 8));
  cellSet.CompleteAddingCells(11);
  return cellSet;
}

void CheckOutputCellSet(const vtkm::cont::CellSetExplicit<>& cellSet,
                        const vtkm::cont::ArrayHandle<vtkm::Float32>& field)
{
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 2, "Wrong num cells.");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPoints() == 5, "Wrong num points.");

  VTKM_TEST_ASSERT(cellSet.GetCellShape(0) == vtkm::CELL_SHAPE_TRIANGLE, "Wrong shape");
  VTKM_TEST_ASSERT(cellSet.GetCellShape(1) == vtkm::CELL_SHAPE_QUAD, "Wrong shape");

  VTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(0) == 3, "Wrong num points");
  VTKM_TEST_ASSERT(cellSet.GetNumberOfPointsInCell(1) == 4, "Wrong num points");

  vtkm::Vec<vtkm::Id, 3> pointIds3;
  cellSet.GetIndices(0, pointIds3);
  VTKM_TEST_ASSERT(pointIds3[0] == 0, "Wrong point id for cell");
  VTKM_TEST_ASSERT(pointIds3[1] == 1, "Wrong point id for cell");
  VTKM_TEST_ASSERT(pointIds3[2] == 2, "Wrong point id for cell");

  vtkm::Vec<vtkm::Id, 4> pointIds4;
  cellSet.GetIndices(1, pointIds4);
  VTKM_TEST_ASSERT(pointIds4[0] == 2, "Wrong point id for cell");
  VTKM_TEST_ASSERT(pointIds4[1] == 1, "Wrong point id for cell");
  VTKM_TEST_ASSERT(pointIds4[2] == 3, "Wrong point id for cell");
  VTKM_TEST_ASSERT(pointIds4[3] == 4, "Wrong point id for cell");

  auto fieldPortal = field.GetPortalConstControl();
  VTKM_TEST_ASSERT(test_equal(fieldPortal.Get(0), TestValue(0, vtkm::Float32())), "Bad field");
  VTKM_TEST_ASSERT(test_equal(fieldPortal.Get(1), TestValue(2, vtkm::Float32())), "Bad field");
  VTKM_TEST_ASSERT(test_equal(fieldPortal.Get(2), TestValue(4, vtkm::Float32())), "Bad field");
  VTKM_TEST_ASSERT(test_equal(fieldPortal.Get(3), TestValue(6, vtkm::Float32())), "Bad field");
  VTKM_TEST_ASSERT(test_equal(fieldPortal.Get(4), TestValue(8, vtkm::Float32())), "Bad field");
}

void RunTest()
{
  using Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;

  std::cout << "Creating input" << std::endl;
  vtkm::cont::CellSetExplicit<> inCellSet = CreateInputCellSet();

  vtkm::cont::ArrayHandle<vtkm::Float32> inField;
  inField.Allocate(inCellSet.GetNumberOfPoints());
  SetPortal(inField.GetPortalControl());

  std::cout << "Removing unused points" << std::endl;
  vtkm::worklet::RemoveUnusedPoints compactPoints(inCellSet, Device());
  vtkm::cont::CellSetExplicit<> outCellSet = compactPoints.MapCellSet(inCellSet, Device());
  vtkm::cont::ArrayHandle<vtkm::Float32> outField =
    compactPoints.MapPointFieldDeep(inField, Device());

  std::cout << "Checking resulting cell set" << std::endl;
  CheckOutputCellSet(outCellSet, outField);
}

} // anonymous namespace

int UnitTestRemoveUnusedPoints(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RunTest);
}
