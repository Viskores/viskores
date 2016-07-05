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

#include <vtkm/worklet/Wavelets.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>


void TestWavelets()
{
  std::cout << "Testing Wavelets Worklet" << std::endl;

  vtkm::Id arraySize = 10;
  std::vector<vtkm::Float32> tmpVector;
  for( vtkm::Id i = 0; i < arraySize; i++ )
    tmpVector.push_back(static_cast<vtkm::Float32>(i));
  
  vtkm::cont::ArrayHandle<vtkm::Float32> input1DArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);
  vtkm::cont::ArrayHandle<vtkm::Float32> output1DArray;


  vtkm::worklet::Wavelets waveletsWorklet;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::Wavelets>
      dispatcher(waveletsWorklet);
  dispatcher.Invoke(input1DArray, output1DArray);


  for (vtkm::Id i = 0; i < output1DArray.GetNumberOfValues(); ++i)
  {
    std::cout<< output1DArray.GetPortalConstControl().Get(i) << std::endl;
    VTKM_TEST_ASSERT(
          test_equal( output1DArray.GetPortalConstControl().Get(i), 
                      static_cast<vtkm::Float32>(i) * 2.0f ),
          "Wrong result for Wavelets worklet");
  }
}

int UnitTestWavelets(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWavelets);
}
