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
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>

static void MultiBlock_TwoDimUniformTest();


void TestMultiBlock_Uniform()
{
  std::cout << std::endl;
  std::cout << "--TestDataSet_Uniform--" << std::endl << std::endl;

  MultiBlock_TwoDimUniformTest();
 
}

static void
MultiBlock_TwoDimUniformTest()
{
  std::cout<<"MultiBlock 2D Uniform data set"<<std::endl;
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::MultiBlock TestBlock;

  vtkm::cont::DataSet dataSet = testDataSet.Make2DUniformDataSet0();

  TestBlock.AddBlock(dataSet);
  TestBlock.AddBlock(dataSet);
  
  vtkm::cont::DataSet TestDSet =TestBlock.GetBlock(1);
  
  VTKM_TEST_ASSERT(TestBlock.GetNumberOfBlocks() == 2,
                   "Incorrect number of blocks");
  VTKM_TEST_ASSERT(dataSet.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(dataSet.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");
  

  // test various field-getting methods and associations
  try
  {
      TestDSet.GetField("cellvar", vtkm::cont::Field::ASSOC_CELL_SET);
  }
  catch (...)
  {
      VTKM_TEST_FAIL("Failed to get field 'cellvar' with ASSOC_CELL_SET.");
  }

  try
  {
    TestDSet.GetField("pointvar", vtkm::cont::Field::ASSOC_POINTS);
  }
  catch (...)
  {
      VTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }
    
}



int UnitTestMultiBlock(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlock_Uniform);
}
