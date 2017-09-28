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
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/MultiBlock.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CellAverage.h>
#include <vtkm/filter/Histogram.h>


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
template <typename T, typename D>
void Result_Verify(T ResultVec, D Filter, vtkm::cont::MultiBlock& Blocks, std::string FieldName)
{
  VTKM_TEST_ASSERT(ResultVec.size() == static_cast<std::size_t>(Blocks.GetNumberOfBlocks()),
                   "result block number incorrect");
  for (vtkm::Id j = 0; static_cast<std::size_t>(j) < ResultVec.size(); j++)
  {
    vtkm::filter::Result BlockResult = Filter.Execute(Blocks.GetBlock(j), FieldName);

    VTKM_TEST_ASSERT(
      ResultVec[static_cast<std::size_t>(j)].GetField().GetData().GetNumberOfValues() ==
        BlockResult.GetField().GetData().GetNumberOfValues(),
      "result vectors' size incorrect");

    vtkm::cont::ArrayHandle<vtkm::Id> MBlockArray;
    ResultVec[static_cast<std::size_t>(j)].GetField().GetData().CopyTo(MBlockArray);
    vtkm::cont::ArrayHandle<vtkm::Id> SDataSetArray;
    BlockResult.GetField().GetData().CopyTo(SDataSetArray);

    for (vtkm::Id i = 0;
         i < ResultVec[static_cast<std::size_t>(j)].GetField().GetData().GetNumberOfValues();
         i++)
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
  std::vector<vtkm::filter::Result> results;
  vtkm::cont::MultiBlock Blocks;

  Blocks = MultiBlockBuilder<vtkm::Float64>(BlockNum, "cellvar");
  vtkm::filter::Histogram histogram;
  results = histogram.Execute(Blocks, std::string("cellvar"));
  Result_Verify(results, histogram, Blocks, std::string("cellvar"));

  Blocks = MultiBlockBuilder<vtkm::Id>(BlockNum, "pointvar");
  vtkm::filter::CellAverage cellAverage;
  results = cellAverage.Execute(Blocks, std::string("pointvar"));

  Result_Verify(results, cellAverage, Blocks, std::string("pointvar"));

  return;
}

int UnitTestMultiBlockFilters(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlockFilters);
}
