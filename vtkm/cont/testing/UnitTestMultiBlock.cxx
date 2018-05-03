//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/thirdparty/diy/Configure.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include VTKM_DIY(diy/master.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

void DataSet_Compare(vtkm::cont::DataSet& LeftDateSet, vtkm::cont::DataSet& RightDateSet);
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

  vtkm::Bounds Set1Bounds = TDset1.GetCoordinateSystem(0).GetBounds();
  vtkm::Bounds Set2Bounds = TDset2.GetCoordinateSystem(0).GetBounds();
  vtkm::Bounds GlobalBound;
  GlobalBound.Include(Set1Bounds);
  GlobalBound.Include(Set2Bounds);

  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(multiblock) == GlobalBound,
                   "Global bounds info incorrect");
  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(multiblock.GetBlock(0)) == Set1Bounds,
                   "Local bounds info incorrect");
  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(multiblock.GetBlock(1)) == Set2Bounds,
                   "Local bounds info incorrect");

  vtkm::Range Set1Field1Range;
  vtkm::Range Set1Field2Range;
  vtkm::Range Set2Field1Range;
  vtkm::Range Set2Field2Range;
  vtkm::Range Field1GlobeRange;
  vtkm::Range Field2GlobeRange;

  TDset1.GetField("pointvar").GetRange(&Set1Field1Range);
  TDset1.GetField("cellvar").GetRange(&Set1Field2Range);
  TDset2.GetField("pointvar").GetRange(&Set2Field1Range);
  TDset2.GetField("cellvar").GetRange(&Set2Field2Range);

  Field1GlobeRange.Include(Set1Field1Range);
  Field1GlobeRange.Include(Set2Field1Range);
  Field2GlobeRange.Include(Set1Field2Range);
  Field2GlobeRange.Include(Set2Field2Range);

  using vtkm::cont::FieldRangeCompute;
  VTKM_TEST_ASSERT(FieldRangeCompute(multiblock, "pointvar").GetPortalConstControl().Get(0) ==
                     Field1GlobeRange,
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(FieldRangeCompute(multiblock, "cellvar").GetPortalConstControl().Get(0) ==
                     Field2GlobeRange,
                   "Local field value range info incorrect");

  vtkm::Range SourceRange; //test the validity of member function GetField(FieldName, BlockId)
  multiblock.GetField("cellvar", 0).GetRange(&SourceRange);
  vtkm::Range TestRange;
  multiblock.GetBlock(0).GetField("cellvar").GetRange(&TestRange);
  VTKM_TEST_ASSERT(TestRange == SourceRange, "Local field value info incorrect");

  vtkm::cont::MultiBlock testblocks1;
  std::vector<vtkm::cont::DataSet> blocks = multiblock.GetBlocks();
  testblocks1.AddBlocks(blocks);
  VTKM_TEST_ASSERT(multiblock.GetNumberOfBlocks() == testblocks1.GetNumberOfBlocks(),
                   "inconsistent number of blocks");

  vtkm::cont::MultiBlock testblocks2(2);
  testblocks2.InsertBlock(0, TDset1);
  testblocks2.InsertBlock(1, TDset2);

  TestDSet = testblocks2.GetBlock(0);
  DataSet_Compare(TDset1, TestDSet);

  TestDSet = testblocks2.GetBlock(1);
  DataSet_Compare(TDset2, TestDSet);

  testblocks2.ReplaceBlock(0, TDset2);
  testblocks2.ReplaceBlock(1, TDset1);

  TestDSet = testblocks2.GetBlock(0);
  DataSet_Compare(TDset2, TestDSet);

  TestDSet = testblocks2.GetBlock(1);
  DataSet_Compare(TDset1, TestDSet);
}

void DataSet_Compare(vtkm::cont::DataSet& LeftDateSet, vtkm::cont::DataSet& RightDateSet)
{
  for (vtkm::Id j = 0; j < LeftDateSet.GetNumberOfFields(); j++)
  {
    vtkm::cont::ArrayHandle<vtkm::Float32> LDataArray;
    LeftDateSet.GetField(j).GetData().CopyTo(LDataArray);
    vtkm::cont::ArrayHandle<vtkm::Float32> RDataArray;
    RightDateSet.GetField(j).GetData().CopyTo(RDataArray);
    VTKM_TEST_ASSERT(LDataArray == RDataArray, "field value info incorrect");
  }
  return;
}

int UnitTestMultiBlock(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(MultiBlockTest);
}
