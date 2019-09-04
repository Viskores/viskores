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
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CellAverage.h>


template <typename T>
vtkm::cont::PartitionedDataSet PartitionedDataSetBuilder(std::size_t partitionNum,
                                                         std::string fieldName)
{
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;

  vtkm::Vec<T, 2> origin(0);
  vtkm::Vec<T, 2> spacing(1);
  vtkm::cont::PartitionedDataSet partitions;
  for (vtkm::Id partId = 0; partId < static_cast<vtkm::Id>(partitionNum); partId++)
  {
    vtkm::Id2 dimensions((partId + 2) * (partId + 2), (partId + 2) * (partId + 2));

    if (fieldName == "cellvar")
    {
      vtkm::Id numCells = (dimensions[0] - 1) * (dimensions[1] - 1);

      std::vector<T> varC2D(static_cast<std::size_t>(numCells));
      for (vtkm::Id i = 0; i < numCells; i++)
      {
        varC2D[static_cast<std::size_t>(i)] = static_cast<T>(partId * i);
      }
      dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                      vtkm::Vec<T, 2>(origin[0], origin[1]),
                                      vtkm::Vec<T, 2>(spacing[0], spacing[1]));
      dsf.AddCellField(dataSet, "cellvar", varC2D);
    }

    if (fieldName == "pointvar")
    {
      vtkm::Id numPoints = dimensions[0] * dimensions[1];
      std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
      for (vtkm::Id i = 0; i < numPoints; i++)
      {
        varP2D[static_cast<std::size_t>(i)] = static_cast<T>(partId);
      }
      dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                      vtkm::Vec<T, 2>(origin[0], origin[1]),
                                      vtkm::Vec<T, 2>(spacing[0], spacing[1]));
      dsf.AddPointField(dataSet, "pointvar", varP2D);
    }

    partitions.AppendPartition(dataSet);
  }
  return partitions;
}
template <typename D>
void Result_Verify(const vtkm::cont::PartitionedDataSet& result,
                   D& filter,
                   const vtkm::cont::PartitionedDataSet& partitions,
                   std::string fieldName)
{
  VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == partitions.GetNumberOfPartitions(),
                   "result partition number incorrect");
  const std::string outputFieldName = filter.GetOutputFieldName();
  for (vtkm::Id j = 0; j < result.GetNumberOfPartitions(); j++)
  {
    filter.SetActiveField(fieldName);
    vtkm::cont::DataSet partitionResult = filter.Execute(partitions.GetPartition(j));

    VTKM_TEST_ASSERT(result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues() ==
                       partitionResult.GetField(outputFieldName).GetNumberOfValues(),
                     "result vectors' size incorrect");

    vtkm::cont::ArrayHandle<vtkm::Id> partitionArray;
    result.GetPartition(j).GetField(outputFieldName).GetData().CopyTo(partitionArray);
    vtkm::cont::ArrayHandle<vtkm::Id> sDataSetArray;
    partitionResult.GetField(outputFieldName).GetData().CopyTo(sDataSetArray);

    const vtkm::Id numValues = result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues();
    for (vtkm::Id i = 0; i < numValues; i++)
    {
      VTKM_TEST_ASSERT(partitionArray.GetPortalConstControl().Get(i) ==
                         sDataSetArray.GetPortalConstControl().Get(i),
                       "result values incorrect");
    }
  }
  return;
}

void TestPartitionedDataSetFilters()
{
  std::size_t partitionNum = 7;
  vtkm::cont::PartitionedDataSet result;
  vtkm::cont::PartitionedDataSet partitions;

  partitions = PartitionedDataSetBuilder<vtkm::Id>(partitionNum, "pointvar");
  vtkm::filter::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("average");
  cellAverage.SetActiveField("pointvar");
  result = cellAverage.Execute(partitions);
  Result_Verify(result, cellAverage, partitions, std::string("pointvar"));
}

int UnitTestPartitionedDataSetFilters(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPartitionedDataSetFilters, argc, argv);
}
