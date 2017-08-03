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

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

static void
MultiBlockTest()
{
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::MultiBlock multiblock;

  vtkm::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  vtkm::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  multiblock.AddBlock(TDset1);
  multiblock.AddBlock(TDset2);

  VTKM_TEST_ASSERT(multiblock.GetNumberOfBlocks() == 2,
                   "Incorrect number of blocks");

  vtkm::cont::DataSet TestDSet =multiblock.GetBlock(0);
  VTKM_TEST_ASSERT(TDset1.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  TestDSet =multiblock.GetBlock(1);
  VTKM_TEST_ASSERT(TDset2.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset2.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  VTKM_TEST_ASSERT(multiblock.GetBounds() == vtkm::Bounds({0,2,0,1,0,2}),
                   "Global bounds info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetBlockBounds(0) == vtkm::Bounds({0,2,0,1,0,0}),
                   "Local bounds info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetBlockBounds(1) == vtkm::Bounds({0,2,0,1,0,2}),
                   "Local bounds info incorrect");

  VTKM_TEST_ASSERT(multiblock.GetGlobalRange("pointvar").GetPortalControl().Get(0) == vtkm::Range({100.1f,200.1f}),
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetGlobalRange("cellvar").GetPortalControl().Get(0) == vtkm::Range({100.1f,200.1f}),
                   "Local field value range info incorrect");

  VTKM_TEST_ASSERT(multiblock.GetGlobalRange(0).GetPortalControl().Get(0) == vtkm::Range({10.1f,180.5f}),
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(multiblock.GetGlobalRange(1).GetPortalControl().Get(0) == vtkm::Range({10.1f,180.5f}),
                   "Local field value range info incorrect");

  vtkm::Range SourceRange;
  multiblock.GetField("cellvar",0).GetRange(&SourceRange);
  VTKM_TEST_ASSERT(SourceRange == vtkm::Range({100.1f,200.1f}),
                   "Local field value info incorrect");
}


int UnitTestMultiBlock(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(MultiBlockTest);
}
