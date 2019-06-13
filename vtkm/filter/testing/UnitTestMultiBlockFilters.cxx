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

#include <vtkm/cont/MultiBlock.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CellAverage.h>


template <typename T>
vtkm::cont::MultiBlock MultiBlockBuilder(std::size_t BlockNum, std::string FieldName)
{
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;

  vtkm::Vec<T, 2> origin(0);
  vtkm::Vec<T, 2> spacing(1);
  vtkm::cont::MultiBlock Blocks;
  for (vtkm::Id BlockId = 0; BlockId < static_cast<vtkm::Id>(BlockNum); BlockId++)
  {
    vtkm::Id2 dimensions((BlockId + 2) * (BlockId + 2), (BlockId + 2) * (BlockId + 2));

    if (FieldName == "cellvar")
    {
      vtkm::Id numCells = (dimensions[0] - 1) * (dimensions[1] - 1);

      std::vector<T> varC2D(static_cast<std::size_t>(numCells));
      for (vtkm::Id i = 0; i < numCells; i++)
      {
        varC2D[static_cast<std::size_t>(i)] = static_cast<T>(BlockId * i);
      }
      dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                      vtkm::Vec<T, 2>(origin[0], origin[1]),
                                      vtkm::Vec<T, 2>(spacing[0], spacing[1]));
      dsf.AddCellField(dataSet, "cellvar", varC2D);
    }

    if (FieldName == "pointvar")
    {
      vtkm::Id numPoints = dimensions[0] * dimensions[1];
      std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
      for (vtkm::Id i = 0; i < numPoints; i++)
      {
        varP2D[static_cast<std::size_t>(i)] = static_cast<T>(BlockId);
      }
      dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                      vtkm::Vec<T, 2>(origin[0], origin[1]),
                                      vtkm::Vec<T, 2>(spacing[0], spacing[1]));
      dsf.AddPointField(dataSet, "pointvar", varP2D);
    }

    Blocks.AddBlock(dataSet);
  }
  return Blocks;
}
template <typename D>
void Result_Verify(const vtkm::cont::MultiBlock& Result,
                   D& Filter,
                   const vtkm::cont::MultiBlock& Blocks,
                   std::string FieldName)
{
  VTKM_TEST_ASSERT(Result.GetNumberOfBlocks() == Blocks.GetNumberOfBlocks(),
                   "result block number incorrect");
  const std::string outputFieldName = Filter.GetOutputFieldName();
  for (vtkm::Id j = 0; j < Result.GetNumberOfBlocks(); j++)
  {
    Filter.SetActiveField(FieldName);
    vtkm::cont::DataSet BlockResult = Filter.Execute(Blocks.GetBlock(j));

    VTKM_TEST_ASSERT(Result.GetBlock(j).GetField(outputFieldName).GetNumberOfValues() ==
                       BlockResult.GetField(outputFieldName).GetNumberOfValues(),
                     "result vectors' size incorrect");

    vtkm::cont::ArrayHandle<vtkm::Id> MBlockArray;
    Result.GetBlock(j).GetField(outputFieldName).GetData().CopyTo(MBlockArray);
    vtkm::cont::ArrayHandle<vtkm::Id> SDataSetArray;
    BlockResult.GetField(outputFieldName).GetData().CopyTo(SDataSetArray);

    for (vtkm::Id i = 0; i < Result.GetBlock(j).GetField(outputFieldName).GetNumberOfValues(); i++)
    {
      VTKM_TEST_ASSERT(MBlockArray.GetPortalConstControl().Get(i) ==
                         SDataSetArray.GetPortalConstControl().Get(i),
                       "result values incorrect");
    }
  }
  return;
}

void TestMultiBlockFilters()
{
  std::size_t BlockNum = 7;
  vtkm::cont::MultiBlock result;
  vtkm::cont::MultiBlock Blocks;

  Blocks = MultiBlockBuilder<vtkm::Id>(BlockNum, "pointvar");
  vtkm::filter::CellAverage cellAverage;
  cellAverage.SetOutputFieldName("average");
  cellAverage.SetActiveField("pointvar");
  result = cellAverage.Execute(Blocks);
  Result_Verify(result, cellAverage, Blocks, std::string("pointvar"));
}

int UnitTestMultiBlockFilters(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlockFilters, argc, argv);
}
