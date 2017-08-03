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

#include <vtkm/CellShape.h>

#include <vtkm/Bounds.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
vtkm::Bounds GlobalBounds(vtkm::cont::MultiBlock multiblock, vtkm::Id CoordSysIndex = 0);
vtkm::Range GlobalRange(vtkm::cont::MultiBlock multiblock, vtkm::Id FieldIndex);
vtkm::Range GlobalRange(vtkm::cont::MultiBlock multiblock, std::string& FieldName);
static void MultiBlockTest()
{
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::MultiBlock multiblock;

  vtkm::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  vtkm::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  multiblock.AddBlock(TDset1);
  multiblock.AddBlock(TDset2);

  VTKM_TEST_ASSERT(multiblock.GetNumberOfBlocks() == 2, "Incorrect number of blocks");

  vtkm::cont::DataSet TestDSet = multiblock.GetBlock(0);
  VTKM_TEST_ASSERT(TDset1.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  TestDSet = multiblock.GetBlock(1);
  VTKM_TEST_ASSERT(TDset2.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset2.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  VTKM_TEST_ASSERT(multiblock.GetBounds() == GlobalBounds(multiblock),
                   "Global bounds info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetBlockBounds(0) ==
                     multiblock.GetBlock(0).GetCoordinateSystem(0).GetBounds(),
                   "Local bounds info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetBlockBounds(1) ==
                     multiblock.GetBlock(1).GetCoordinateSystem(0).GetBounds(),
                   "Local bounds info incorrect");

  VTKM_TEST_ASSERT(multiblock.GetGlobalRange("pointvar").GetPortalControl().Get(0) ==
                     GlobalRange(multiblock, "pointvar"),
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetGlobalRange("cellvar").GetPortalControl().Get(0) ==
                     GlobalRange(multiblock, "cellvar"),
                   "Local field value range info incorrect");

  VTKM_TEST_ASSERT(multiblock.GetGlobalRange(0).GetPortalControl().Get(0) ==
                     GlobalRange(multiblock, 0),
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetGlobalRange(1).GetPortalControl().Get(0) ==
                     GlobalRange(multiblock, 0),
                   "Local field value range info incorrect");

  vtkm::Range SourceRange;
  multiblock.GetField("cellvar", 0).GetRange(&SourceRange);
  VTKM_TEST_ASSERT(SourceRange == multiblock.GetBlock(0).GetField("cellvar").GetRange(),
                   "Local field value info incorrect");
}

vtkm::Bounds GlobalBounds(vtkm::cont::MultiBlock multiblock, vtkm::Id CoordSysIndex = 0)
{
  vtkm::Bounds bounds;
  for (vtkm::Id i = 0; i < multiblock.GetNumberOfBlocks(); ++i)
  {
    vtkm::Bounds block_bounds =
      multiblock.GetBlock(i).GetCoordinateSystem(CoordSysIndex).GetBounds();
    bounds.Include(block_bounds);
  }
  return bounds;
}

vtkm::Range GlobalRange(vtkm::cont::MultiBlock multiblock, vtkm::Id FieldIndex)
{
  vtkm::Range range;
  for (vtkm::Id i = 0; i < multiblock.GetNumberOfBlocks(); ++i)
  {
    vtkm::Range block_range = multiblock.GetBlock(i).GetField(FieldIndex).GetRange();
    range.Include(block_range);
  }
  return range;
}

vtkm::Range GlobalRange(vtkm::cont::MultiBlock multiblock, std::string& FieldName)
{
  vtkm::Range range;
  for (vtkm::Id i = 0; i < multiblock.GetNumberOfBlocks(); ++i)
  {
    vtkm::Range block_range = multiblock.GetBlock(i).GetField(FieldName).GetRange();
    range.Include(block_range);
  }
  return range;
}

int UnitTestMultiBlock(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(MultiBlockTest);
}
