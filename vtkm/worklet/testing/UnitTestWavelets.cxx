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

  vtkm::Id signalLen = 20;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < signalLen + 9; i++ )
    tmpVector.push_back( i + 1 );
 
  vtkm::cont::ArrayHandle<vtkm::Float64> input1DArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);
  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray1;

  // make two filter array handles
  vtkm::cont::ArrayHandle<vtkm::Float64> lowFilter = 
    vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::hm4_44, 9);
  vtkm::cont::ArrayHandle<vtkm::Float64> highFilter = 
    vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::h4, 9);


  vtkm::worklet::Wavelets::ForwardTransform forwardTransform;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::Wavelets::ForwardTransform> 
    dispatcher(forwardTransform);
  dispatcher.Invoke(input1DArray, 
                    lowFilter, 
                    highFilter,
                    outputArray1);
  std::cerr << "Invoke succeeded; output has length: " << 
    outputArray1.GetNumberOfValues() << std::endl;

  for (vtkm::Id i = 0; i < outputArray1.GetNumberOfValues(); ++i)
  {
    std::cout << outputArray1.GetPortalConstControl().Get(i) << ", ";
    if( i % 2 != 0 )
      std::cout << std::endl;
  }
}

int UnitTestWavelets(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWavelets);
}
