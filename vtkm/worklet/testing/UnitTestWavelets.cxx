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


void TestWavelets()
{
  std::cout << "Testing Wavelets Worklet" << std::endl;

  vtkm::Id sigLen = 5000;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen + 8; i++ )
    tmpVector.push_back( i + 1 );
 
  vtkm::cont::ArrayHandle<vtkm::Float64> input1DArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);
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
  vtkm::worklet::Wavelets::ForwardTransform forwardTransform;
  forwardTransform.SetFilterLength( 9 );
  forwardTransform.SetCoeffLength( sigLen/2, sigLen/2 );
  forwardTransform.SetOddness( false, true );

  // setup a timer
  vtkm::cont::Timer<> timer;

  vtkm::worklet::DispatcherMapField<vtkm::worklet::Wavelets::ForwardTransform> 
    dispatcher(forwardTransform);
  dispatcher.Invoke(input1DArray, 
                    lowFilter, 
                    highFilter,
                    outputArray1);

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cerr << "Invoke succeeded; time elapsed = " << elapsedTime << std::endl;

  /*
  for (vtkm::Id i = 0; i < outputArray1.GetNumberOfValues(); ++i)
  {
    std::cout << outputArray1.GetPortalConstControl().Get(i) << ", ";
    if( i % 2 != 0 )
      std::cout << std::endl;
  }
  */
}

int UnitTestWavelets(int argc, char* argv[])
{
  std::cout << "argc = " << argc << std::endl;
  return vtkm::cont::testing::Testing::Run(TestWavelets);
}
