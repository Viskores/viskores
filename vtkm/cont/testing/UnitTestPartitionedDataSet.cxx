//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/CellShape.h>

#include <vtkm/Bounds.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/thirdparty/diy/Configure.h>

#include <vtkm/thirdparty/diy/diy.h>

void DataSet_Compare(vtkm::cont::DataSet& LeftDateSet, vtkm::cont::DataSet& RightDateSet);
static void PartitionedDataSetTest()
{
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::PartitionedDataSet pds;

  vtkm::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  vtkm::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  pds.AppendPartition(TDset1);
  pds.AppendPartition(TDset2);

  VTKM_TEST_ASSERT(pds.GetNumberOfPartitions() == 2, "Incorrect number of partitions");

  vtkm::cont::DataSet TestDSet = pds.GetPartition(0);
  VTKM_TEST_ASSERT(TDset1.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  TestDSet = pds.GetPartition(1);
  VTKM_TEST_ASSERT(TDset2.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset2.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  vtkm::Bounds Set1Bounds = TDset1.GetCoordinateSystem(0).GetBounds();
  vtkm::Bounds Set2Bounds = TDset2.GetCoordinateSystem(0).GetBounds();
  vtkm::Bounds GlobalBound;
  GlobalBound.Include(Set1Bounds);
  GlobalBound.Include(Set2Bounds);

  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(pds) == GlobalBound, "Global bounds info incorrect");
  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(pds.GetPartition(0)) == Set1Bounds,
                   "Local bounds info incorrect");
  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(pds.GetPartition(1)) == Set2Bounds,
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
  VTKM_TEST_ASSERT(FieldRangeCompute(pds, "pointvar").GetPortalConstControl().Get(0) ==
                     Field1GlobeRange,
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(FieldRangeCompute(pds, "cellvar").GetPortalConstControl().Get(0) ==
                     Field2GlobeRange,
                   "Local field value range info incorrect");

  vtkm::Range SourceRange; //test the validity of member function GetField(FieldName, BlockId)
  pds.GetField("cellvar", 0).GetRange(&SourceRange);
  vtkm::Range TestRange;
  pds.GetPartition(0).GetField("cellvar").GetRange(&TestRange);
  VTKM_TEST_ASSERT(TestRange == SourceRange, "Local field value info incorrect");

  vtkm::cont::PartitionedDataSet testblocks1;
  std::vector<vtkm::cont::DataSet> partitions = pds.GetPartitions();
  testblocks1.AppendPartitions(partitions);
  VTKM_TEST_ASSERT(pds.GetNumberOfPartitions() == testblocks1.GetNumberOfPartitions(),
                   "inconsistent number of partitions");

  vtkm::cont::PartitionedDataSet testblocks2(2);
  testblocks2.InsertPartition(0, TDset1);
  testblocks2.InsertPartition(1, TDset2);

  TestDSet = testblocks2.GetPartition(0);
  DataSet_Compare(TDset1, TestDSet);

  TestDSet = testblocks2.GetPartition(1);
  DataSet_Compare(TDset2, TestDSet);

  testblocks2.ReplacePartition(0, TDset2);
  testblocks2.ReplacePartition(1, TDset1);

  TestDSet = testblocks2.GetPartition(0);
  DataSet_Compare(TDset2, TestDSet);

  TestDSet = testblocks2.GetPartition(1);
  DataSet_Compare(TDset1, TestDSet);
}

void DataSet_Compare(vtkm::cont::DataSet& leftDataSet, vtkm::cont::DataSet& rightDataSet)
{
  for (vtkm::Id j = 0; j < leftDataSet.GetNumberOfFields(); j++)
  {
    vtkm::cont::ArrayHandle<vtkm::Float32> lDataArray;
    leftDataSet.GetField(j).GetData().CopyTo(lDataArray);
    vtkm::cont::ArrayHandle<vtkm::Float32> rDataArray;
    rightDataSet.GetField(j).GetData().CopyTo(rDataArray);
    VTKM_TEST_ASSERT(lDataArray == rDataArray, "field value info incorrect");
  }
  return;
}

int UnitTestPartitionedDataSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(PartitionedDataSetTest, argc, argv);
}
