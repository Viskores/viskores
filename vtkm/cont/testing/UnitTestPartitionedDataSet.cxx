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

  std::vector<vtkm::Id> ids = { 0, 1 };
  std::vector<vtkm::FloatDefault> var = { 1, 2 };
  auto idsField = vtkm::cont::make_Field(
    "ids", vtkm::cont::Field::Association::Partitions, ids, vtkm::CopyFlag::On);
  auto pdsVar = vtkm::cont::make_Field(
    "pds_var", vtkm::cont::Field::Association::Partitions, ids, vtkm::CopyFlag::On);
  pds.AddField(idsField);
  pds.AddField(pdsVar);

  VTKM_TEST_ASSERT(pds.GetNumberOfPartitions() == 2, "Incorrect number of partitions");
  VTKM_TEST_ASSERT(pds.GetNumberOfFields() == 2, "Incorrect number of fields");

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
  VTKM_TEST_ASSERT(FieldRangeCompute(pds, "pointvar").ReadPortal().Get(0) == Field1GlobeRange,
                   "Local field value range info incorrect");
  VTKM_TEST_ASSERT(FieldRangeCompute(pds, "cellvar").ReadPortal().Get(0) == Field2GlobeRange,
                   "Local field value range info incorrect");

  vtkm::Range SourceRange; //test the validity of member function GetField(FieldName, BlockId)
  pds.GetFieldFromPartition("cellvar", 0).GetRange(&SourceRange);
  vtkm::Range TestRange;
  pds.GetPartition(0).GetField("cellvar").GetRange(&TestRange);
  VTKM_TEST_ASSERT(TestRange == SourceRange, "Local field value info incorrect");

  //test partition fields.
  idsField.GetRange(&SourceRange);
  pds.GetField("ids").GetRange(&TestRange);
  VTKM_TEST_ASSERT(TestRange == SourceRange, "Partitions field values incorrect");

  pdsVar.GetRange(&SourceRange);
  pds.GetField("pds_var").GetRange(&TestRange);
  VTKM_TEST_ASSERT(TestRange == SourceRange, "Global field values incorrect");

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

static void PartitionedDataSetFieldTest()
{
  vtkm::cont::testing::MakeTestDataSet testDataSet;

  vtkm::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  vtkm::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  constexpr vtkm::Id id0 = 0, id1 = 1;
  constexpr vtkm::FloatDefault globalScalar = 1.0f;

  for (int i = 0; i < 4; i++)
  {
    vtkm::cont::PartitionedDataSet pds({ TDset1, TDset2 });
    std::vector<vtkm::Id> ids = { id0, id1 };
    std::vector<vtkm::FloatDefault> gs = { globalScalar };

    auto idsArr = vtkm::cont::make_ArrayHandle(ids, vtkm::CopyFlag::Off);
    auto gsArr = vtkm::cont::make_ArrayHandle(gs, vtkm::CopyFlag::Off);

    if (i == 0) //field
    {
      auto idField = vtkm::cont::make_Field(
        "id", vtkm::cont::Field::Association::Partitions, ids, vtkm::CopyFlag::Off);
      auto gScalar = vtkm::cont::make_Field(
        "global_scalar", vtkm::cont::Field::Association::Global, gs, vtkm::CopyFlag::Off);

      pds.AddField(idField);
      pds.AddField(gScalar);
    }
    else if (i == 1) //array handle
    {
      pds.AddPartitionsField("id", idsArr);
      pds.AddGlobalField("global_scalar", gsArr);
    }
    else if (i == 2) //std::vector
    {
      pds.AddPartitionsField("id", ids);
      pds.AddGlobalField("global_scalar", gs);
    }
    else if (i == 3) //pointer
    {
      pds.AddPartitionsField("id", ids.data(), 2);
      pds.AddGlobalField("global_scalar", gs.data(), 1);
    }

    //Validate each method.
    VTKM_TEST_ASSERT(pds.GetNumberOfFields() == 2, "Wrong number of fields");

    //Make sure fields are there and of the right type.
    VTKM_TEST_ASSERT(pds.HasPartitionsField("id"), "id field misssing.");
    VTKM_TEST_ASSERT(pds.HasGlobalField("global_scalar"), "global_scalar field misssing.");


    for (int j = 0; j < 2; j++)
    {
      vtkm::cont::Field f0, f1;

      if (j == 0)
      {
        f0 = pds.GetField("id");
        f1 = pds.GetField("global_scalar");
      }
      else
      {
        f0 = pds.GetPartitionsField("id");
        f1 = pds.GetGlobalField("global_scalar");
      }

      //Check the values.
      auto portal0 = f0.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>().ReadPortal();
      auto portal1 =
        f1.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>().ReadPortal();

      VTKM_TEST_ASSERT(portal0.GetNumberOfValues() == 2, "Wrong number of values in field");
      VTKM_TEST_ASSERT(portal1.GetNumberOfValues() == 1, "Wrong number of values in field");

      VTKM_TEST_ASSERT(portal0.Get(0) == id0 && portal0.Get(1) == id1, "Wrong field value");
      VTKM_TEST_ASSERT(portal1.Get(0) == globalScalar, "Wrong field value");
    }
  }
}

void DataSet_Compare(vtkm::cont::DataSet& leftDataSet, vtkm::cont::DataSet& rightDataSet)
{
  for (vtkm::Id j = 0; j < leftDataSet.GetNumberOfFields(); j++)
  {
    if (leftDataSet.HasCoordinateSystem(leftDataSet.GetField(j).GetName()))
    {
      // Skip coordinate systems, which have a different array type.
      continue;
    }
    vtkm::cont::ArrayHandle<vtkm::Float32> lDataArray;
    leftDataSet.GetField(j).GetData().AsArrayHandle(lDataArray);
    vtkm::cont::ArrayHandle<vtkm::Float32> rDataArray;
    rightDataSet.GetField(j).GetData().AsArrayHandle(rDataArray);
    VTKM_TEST_ASSERT(lDataArray == rDataArray, "field value info incorrect");
  }
  return;
}

static void PartitionedDataSetTests()
{
  PartitionedDataSetTest();
  PartitionedDataSetFieldTest();
}

int UnitTestPartitionedDataSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(PartitionedDataSetTests, argc, argv);
}
