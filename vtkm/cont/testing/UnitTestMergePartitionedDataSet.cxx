//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Bounds.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/MergePartitionedDataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

static void MergePartitionedDataSetTest()
{
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::PartitionedDataSet pds;

  vtkm::cont::DataSet TDset1 = testDataSet.Make2DUniformDataSet0();
  vtkm::cont::DataSet TDset2 = testDataSet.Make3DUniformDataSet0();

  pds.AppendPartition(TDset1);
  pds.AppendPartition(TDset2);

  vtkm::cont::DataSet mergedDataset = vtkm::cont::MergePartitionedDataSet(pds);

  VTKM_TEST_ASSERT(TDset1.GetNumberOfFields() == mergedDataset.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset2.GetNumberOfFields() == mergedDataset.GetNumberOfFields(),
                   "Incorrect number of fields");

  VTKM_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() ==
                     mergedDataset.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  vtkm::Bounds Set1Bounds = TDset1.GetCoordinateSystem(0).GetBounds();
  vtkm::Bounds Set2Bounds = TDset2.GetCoordinateSystem(0).GetBounds();
  vtkm::Bounds GlobalBound;
  GlobalBound.Include(Set1Bounds);
  GlobalBound.Include(Set2Bounds);

  VTKM_TEST_ASSERT(vtkm::cont::BoundsCompute(mergedDataset) == GlobalBound,
                   "Global bounds info incorrect");

  vtkm::Range MergedField1Range;
  vtkm::Range MergedField2Range;
  vtkm::Range Set1Field1Range;
  vtkm::Range Set1Field2Range;
  vtkm::Range Set2Field1Range;
  vtkm::Range Set2Field2Range;
  vtkm::Range Field1GlobeRange;
  vtkm::Range Field2GlobeRange;

  mergedDataset.GetField("pointvar").GetRange(&MergedField1Range);
  mergedDataset.GetField("cellvar").GetRange(&MergedField2Range);
  TDset1.GetField("pointvar").GetRange(&Set1Field1Range);
  TDset1.GetField("cellvar").GetRange(&Set1Field2Range);
  TDset2.GetField("pointvar").GetRange(&Set2Field1Range);
  TDset2.GetField("cellvar").GetRange(&Set2Field2Range);

  Field1GlobeRange.Include(Set1Field1Range);
  Field1GlobeRange.Include(Set2Field1Range);
  Field2GlobeRange.Include(Set1Field2Range);
  Field2GlobeRange.Include(Set2Field2Range);

  using vtkm::cont::FieldRangeCompute;
  VTKM_TEST_ASSERT(MergedField1Range == Field1GlobeRange, "Local field value range info incorrect");
  VTKM_TEST_ASSERT(MergedField2Range == Field2GlobeRange, "Local field value range info incorrect");

  VTKM_TEST_ASSERT(mergedDataset.GetNumberOfPoints() ==
                     TDset1.GetNumberOfPoints() + TDset2.GetNumberOfPoints(),
                   "Incorrect number of points");
  VTKM_TEST_ASSERT(mergedDataset.GetNumberOfCells() ==
                     TDset1.GetNumberOfCells() + TDset2.GetNumberOfCells(),
                   "Incorrect number of cells");
}

int UnitTestMergePartitionedDataSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(MergePartitionedDataSetTest, argc, argv);
}
