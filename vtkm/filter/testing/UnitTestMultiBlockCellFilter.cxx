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

#include <vtkm/filter/CellAverage.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/Histogram.h>


const std::vector<vtkm::filter::ResultField> MultiBlockCellTest(std::size_t BlockNum);

void TestMultiBlockCell()
{
  std::size_t BlockNum=7;
  std::vector<vtkm::filter::ResultField> results = MultiBlockCellTest( BlockNum );
  VTKM_TEST_ASSERT(results.size() == BlockNum, "result block number incorrect");
  for(std::size_t j = 0; j < results.size(); j++)
  { 
    VTKM_TEST_ASSERT(results[j].GetField().GetData().GetNumberOfValues() == ((j+2)*(j+2)-1)
    *((j+2)*(j+2)-1), "result cell size incorrect");

    vtkm::cont::ArrayHandle<vtkm::Float64> array; 
    results[j].GetField().GetData().CopyTo(array);
    for(std::size_t i = 0; i < results[j].GetField().GetData().GetNumberOfValues(); i++)
    { 
      VTKM_TEST_ASSERT(array.GetPortalConstControl().Get(i) == j,
      "field value incorrect");
    }
  }
 

}



template <typename T>
vtkm::cont::MultiBlock UniformMultiBlockBuilder(std::size_t BlockNum)
{
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::Vec<T,2> origin(0);
  vtkm::Vec<T,2> spacing(1);
  vtkm::cont::MultiBlock Blocks;
  for (vtkm::Id BlockId = 0; BlockId < BlockNum; BlockId++)
  {
    vtkm::Id2 dimensions( (BlockId+2) * (BlockId+2), (BlockId+2) * (BlockId+2) );
    vtkm::Id numPoints = dimensions[0] * dimensions[1];
    vtkm::Id numCells = (dimensions[0]-1) * (dimensions[1]-1);
    std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    {
      varP2D[i] = static_cast<T>(BlockId);
    }
   
    dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                    vtkm::Vec<T,2>(origin[0], origin[1]),
                                    vtkm::Vec<T,2>(spacing[0], spacing[1]));
    dsf.AddPointField(dataSet, "pointvar", varP2D);
    Blocks.AddBlock(dataSet);
  }
  return Blocks;
}



const std::vector<vtkm::filter::ResultField> MultiBlockCellTest(std::size_t BlockNum)
{
  vtkm::cont::MultiBlock Blocks = UniformMultiBlockBuilder<vtkm::Float64>(BlockNum);
  std::vector<vtkm::filter::ResultField> results;
  vtkm::filter::CellAverage cellAverage;
  results = cellAverage.Execute(Blocks, std::string("pointvar"));
  return results;
}



int UnitTestMultiBlockCellFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlockCell);
}
