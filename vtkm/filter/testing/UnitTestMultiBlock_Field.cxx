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


#include <vtkm/filter/Histogram.h>

const std::vector<vtkm::filter::ResultField> MultiBlock_FieldFilterTest();

void TestMultiBlock_Field()
{
  std::cout << std::endl;
  std::vector<vtkm::filter::ResultField> results=MultiBlock_FieldFilterTest();
  for(std::size_t j=0; j<results.size(); j++)
  { 
    results[j].GetField().PrintSummary(std::cout);
  }
}




template <typename T>
vtkm::cont::MultiBlock UniformMultiBlockBuilder()
{
  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::Vec<T,3> origin(0);
  vtkm::Vec<T,3> spacing(1);
  vtkm::cont::MultiBlock Blocks;
  for (vtkm::Id trial = 0; trial < 7; trial++)
  {
    vtkm::Id3 dimensions(10, 10, 10);
    vtkm::Id numPoints = dimensions[0] * dimensions[1];
    vtkm::Id numCells = (dimensions[0]-1) * (dimensions[1]-1);
    std::vector<T> varP2D(static_cast<std::size_t>(numPoints));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    {
      //varP2D[i] = static_cast<T>((trial-1)*i);
      varP2D[i] = static_cast<T>(trial);
    }
    std::vector<T> varC2D(static_cast<std::size_t>(numCells));
    for (std::size_t i = 0; i < static_cast<std::size_t>(numCells); i++)
    {
      varC2D[i] = static_cast<T>(trial*i);
    }
    dataSet = dataSetBuilder.Create(vtkm::Id2(dimensions[0], dimensions[1]),
                                    vtkm::Vec<T,2>(origin[0], origin[1]),
                                    vtkm::Vec<T,2>(spacing[0], spacing[1]));
    dsf.AddPointField(dataSet, "pointvar", varP2D);
    dsf.AddCellField(dataSet, "cellvar", varC2D);
    Blocks.AddBlock(dataSet);
  }
  return Blocks;
}



const std::vector<vtkm::filter::ResultField> MultiBlock_FieldFilterTest()
{
  vtkm::cont::MultiBlock Blocks=UniformMultiBlockBuilder<vtkm::Float64>();
  
  vtkm::filter::Histogram histogram;
  std::vector<vtkm::filter::ResultField> results;

  results = histogram.Execute(Blocks, std::string("pointvar"));
  return results;
}



int UnitTestMultiBlock_Field(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlock_Field);
}
