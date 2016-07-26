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

#include <vtkm/worklet/WaveletTransforms.h>
#include <vtkm/worklet/DispatcherMapField.h>


#include <vtkm/filter/internal/FilterBanks.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/Timer.h>

#include <vector>


void TestWaveletTransforms( )
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Default test size is 20. " << std::endl;
  std::cout << "Input a new size to test (in millions)." << std::endl;
  std::cout << "Input 0 to stick with 20." << std::endl;
  vtkm::Id tmpIn;
  vtkm::Id million = 1000000;
  std::cin >> tmpIn;
  if( tmpIn != 0 )
    sigLen = tmpIn * million;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen + 8; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>(i%100+1) );
 
  vtkm::cont::ArrayHandle<vtkm::Float64> input1DArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  // output array handle
  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray1;

  // make two filter array handles
  vtkm::cont::ArrayHandle<vtkm::Float64> lowFilter = 
    vtkm::cont::make_ArrayHandle(vtkm::filter::internal::hm4_44, 9);
  vtkm::cont::ArrayHandle<vtkm::Float64> highFilter = 
    vtkm::cont::make_ArrayHandle(vtkm::filter::internal::h4, 9);

  // initialize a worklet
  vtkm::worklet::ForwardTransform forwardTransform;
  forwardTransform.SetFilterLength( 9 );
  forwardTransform.SetCoeffLength( sigLen/2, sigLen/2 );
  forwardTransform.SetOddness( false, true );

  // setup a timer
  vtkm::cont::Timer<> timer;

  vtkm::worklet::DispatcherMapField<vtkm::worklet::ForwardTransform> 
    dispatcher(forwardTransform);
  dispatcher.Invoke(input1DArray, 
                    lowFilter, 
                    highFilter,
                    outputArray1);

  srand ((unsigned int)time(NULL));
  vtkm::Id randNum = rand() % sigLen;
  std::cout << "A random output: " 
            << outputArray1.GetPortalConstControl().Get(randNum) << std::endl;

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cerr << "Dealing array size " << sigLen/million << " millions takes time " 
            << elapsedTime << std::endl;
  if( sigLen < 21 )
    for (vtkm::Id i = 0; i < outputArray1.GetNumberOfValues(); ++i)
    {
      std::cout << outputArray1.GetPortalConstControl().Get(i) << ", ";
      if( i % 2 != 0 )
        std::cout << std::endl;
    }
}


int UnitTestWaveletTransforms(int, char* [])
{
  // TestDWT1D();
  // TestExtend1D();
  return vtkm::cont::testing::Testing::Run(TestWaveletTransforms);

  return 0;
}
