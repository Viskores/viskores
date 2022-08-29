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
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/field_conversion/CellAverage.h>


template <typename T>
vtkm::cont::PartitionedDataSet PartitionedDataSetBuilder(std::size_t partitionNum,
                                                         std::string fieldName)
{
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;

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
      dataSet.AddCellField("cellvar", varC2D);
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
      dataSet.AddPointField("pointvar", varP2D);
    }

    partitions.AppendPartition(dataSet);
  }
  return partitions;
}
template <typename T, typename D>
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

    vtkm::cont::ArrayHandle<T> partitionArray;
    result.GetPartition(j).GetField(outputFieldName).GetData().AsArrayHandle(partitionArray);
    vtkm::cont::ArrayHandle<T> sDataSetArray;
    partitionResult.GetField(outputFieldName).GetData().AsArrayHandle(sDataSetArray);

    const vtkm::Id numValues = result.GetPartition(j).GetField(outputFieldName).GetNumberOfValues();
    for (vtkm::Id i = 0; i < numValues; i++)
    {
      VTKM_TEST_ASSERT(partitionArray.ReadPortal().Get(i) == sDataSetArray.ReadPortal().Get(i),
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

  partitions = PartitionedDataSetBuilder<vtkm::FloatDefault>(partitionNum, "pointvar");
  vtkm::filter::field_conversion::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("average");
  cellAverage.SetActiveField("pointvar");
  result = cellAverage.Execute(partitions);
  Result_Verify<vtkm::FloatDefault>(result, cellAverage, partitions, std::string("pointvar"));

  //Make sure that any Fields are propagated to the output.
  //Test it with and without using SetFieldsToPass
  std::vector<std::vector<std::string>> fieldsToPass;
  fieldsToPass.push_back({});
  fieldsToPass.push_back({ "ids" });
  fieldsToPass.push_back({ "scalar" });
  fieldsToPass.push_back({ "ids", "scalar" });

  for (auto& fields : fieldsToPass)
  {
    partitionNum = 3;
    partitions = PartitionedDataSetBuilder<vtkm::FloatDefault>(partitionNum, "pointvar");
    std::vector<vtkm::Id> ids = { 0, 1, 2 };
    std::vector<vtkm::FloatDefault> scalar = { 10.0f };
    partitions.AddPartitionsField("ids", ids);
    partitions.AddGlobalField("scalar", scalar);

    //On second iteration, only allow "ids" to pass through.
    cellAverage.GetFieldsToPass().ClearFields();
    if (!fields.empty())
    {
      cellAverage.GetFieldsToPass().SetMode(vtkm::filter::FieldSelection::Mode::Select);
      for (auto& f : fields)
        cellAverage.GetFieldsToPass().AddField(f);
    }

    result = cellAverage.Execute(partitions);

    if (fields.empty() || std::find(fields.begin(), fields.end(), "ids") != fields.end())
    {
      VTKM_TEST_ASSERT(result.HasPartitionsField("ids"), "Missing field on result");
      auto field0 = result.GetField("ids");
      auto portal0 =
        field0.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>().ReadPortal();
      VTKM_TEST_ASSERT(portal0.GetNumberOfValues() == static_cast<vtkm::Id>(ids.size()),
                       "Wrong number of field values.");
      for (std::size_t i = 0; i < ids.size(); i++)
        VTKM_TEST_ASSERT(portal0.Get(static_cast<vtkm::Id>(i)) == ids[i], "Wrong field value.");
    }
    else
    {
      VTKM_TEST_ASSERT(!result.HasPartitionsField("ids"), "Field should not be on result");
    }

    if (fields.empty() || std::find(fields.begin(), fields.end(), "scalar") != fields.end())
    {
      VTKM_TEST_ASSERT(result.HasGlobalField("scalar"), "Missing field on result");
      auto field1 = result.GetField("scalar");
      auto portal1 =
        field1.GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>().ReadPortal();
      VTKM_TEST_ASSERT(portal1.GetNumberOfValues() == static_cast<vtkm::Id>(scalar.size()),
                       "Wrong number of field values.");
      VTKM_TEST_ASSERT(portal1.Get(0) == scalar[0], "Wrong field value.");
    }
    else
    {
      VTKM_TEST_ASSERT(!result.HasGlobalField("scalar"), "Field should not be on result");
    }
  }
}

int UnitTestPartitionedDataSetFilters(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPartitionedDataSetFilters, argc, argv);
}
