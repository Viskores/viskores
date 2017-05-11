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
  std::cout << "--TestDataSet Uniform and Rectilinear--" << std::endl << std::endl;
  MultiBlock_TwoDimUniformTest();
}

static void
MultiBlock_TwoDimUniformTest()
{ 
  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::MultiBlock TestBlock;  
    
  vtkm::cont::DataSet TDset1=testDataSet.Make2DUniformDataSet0();
  vtkm::cont::DataSet TDset2=testDataSet.Make3DUniformDataSet0();

  TestBlock.AddBlock(TDset1);
  TestBlock.AddBlock(TDset2);
   
  VTKM_TEST_ASSERT(TestBlock.GetNumberOfBlocks() == 2,
                   "Incorrect number of blocks");

  vtkm::cont::DataSet TestDSet =TestBlock.GetBlock(0);
  VTKM_TEST_ASSERT(TDset1.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset1.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");

  TestDSet =TestBlock.GetBlock(1);
  VTKM_TEST_ASSERT(TDset2.GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(TDset2.GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");
 
  std::vector<vtkm::cont::DataSet> Vblocks;
  Vblocks.push_back(testDataSet.Make2DRectilinearDataSet0());
  Vblocks.push_back(testDataSet.Make3DRegularDataSet1());
  Vblocks.push_back(testDataSet.Make3DRegularDataSet0());
  Vblocks.push_back(testDataSet.Make3DExplicitDataSet4());

  vtkm::cont::MultiBlock T2Block(Vblocks);
  std::vector<vtkm::cont::DataSet> InBlocks = T2Block.GetBlocks();
  for(int j=0; j<InBlocks.size(); j++)
  {
    vtkm::cont::DataSet TestDSet =InBlocks[j];
    VTKM_TEST_ASSERT(Vblocks[j].GetNumberOfFields() == TestDSet.GetNumberOfFields(),
                   "Incorrect number of fields");
    VTKM_TEST_ASSERT(Vblocks[j].GetNumberOfCoordinateSystems() == TestDSet.GetNumberOfCoordinateSystems(),
                   "Incorrect number of coordinate systems");
  }  
}



int UnitTestMultiBlock(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlock_Uniform);
}
