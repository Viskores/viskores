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
#include <vtkm/cont/Timer.h>

#include <vector>


void TestWavelets( )
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
    vtkm::cont::make_ArrayHandle(vtkm::worklet::wavelet::hm4_44, 9);
  vtkm::cont::ArrayHandle<vtkm::Float64> highFilter = 
    vtkm::cont::make_ArrayHandle(vtkm::worklet::wavelet::h4, 9);

  // make a wavelet filter
  std::string wname = "CDF9/7";
  vtkm::worklet::wavelet::WaveletFilter CDF97( wname );

  // initialize the worklet
  vtkm::worklet::wavelet::Wavelets::ForwardTransform forwardTransform;
  forwardTransform.SetFilterLength( 9 );
  forwardTransform.SetCoeffLength( sigLen/2, sigLen/2 );
  forwardTransform.SetOddness( false, true );

  // setup a timer
  srand (time(NULL));
  vtkm::cont::Timer<> timer;

  vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelet::Wavelets::ForwardTransform> 
    dispatcher(forwardTransform);
  dispatcher.Invoke(input1DArray, 
                    lowFilter, 
                    highFilter,
                    outputArray1);

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

void TestExtend1D()
{
  // make input data array handle
  vtkm::Id sigLen = 20;
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>(i) );
 
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::worklet::wavelet::Wavelets w;
  typedef vtkm::Float64 T;
  typedef vtkm::cont::ArrayHandle<T>     ArrayType;
  typedef vtkm::cont::ArrayHandleConcatenate< ArrayType, ArrayType> 
            ArrayConcat;
  typedef vtkm::cont::ArrayHandleConcatenate< ArrayConcat, ArrayType > ArrayConcat2;
  
  ArrayConcat2 outputArray;
  w.Extend1D( inputArray, outputArray, 4, 
      vtkm::worklet::wavelet::SYMW, vtkm::worklet::wavelet::SYMW );

  std::cout << "Start testing Extend1D" << std::endl;
  for (vtkm::Id i = 0; i < outputArray.GetNumberOfValues(); ++i)
      std::cout << outputArray.GetPortalConstControl().Get(i) << std::endl;
  std::cout << "\nFinish testing Extend1D" << std::endl;
}

int UnitTestWavelets(int, char* [])
{

  TestExtend1D();
  return vtkm::cont::testing::Testing::Run(TestWavelets);
}
