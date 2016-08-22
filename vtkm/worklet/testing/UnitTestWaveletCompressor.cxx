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

#include <vtkm/worklet/WaveletCompressor.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/Timer.h>

#include <vector>
#include <iomanip>

void DebugDWTIDWT1D()
{
  vtkm::Id sigLen = 21;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Input a size to test." << std::endl;
  std::cin >> sigLen;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>( i ) );
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> coeffOut;
  std::vector<vtkm::Id> L(3, 0);

  // Forward Transform
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF8_4;
  vtkm::worklet::wavelets::WaveletDWT waveletdwt( wname );
  waveletdwt.DWT1D( inputArray, coeffOut, L );

  std::cout << "Forward Wavelet Transform: result coeff length = " << 
      coeffOut.GetNumberOfValues() << std::endl;

  for( vtkm::Id i = 0; i < coeffOut.GetNumberOfValues(); i++ )
  {
    if( i == 0 )
      std::cout << "  <-- cA --> " << std::endl;
    else if( i == L[0] )
      std::cout << "  <-- cD --> " << std::endl;
    std::cout << coeffOut.GetPortalConstControl().Get(i) << std::endl;
  }

  // Inverse Transform
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  waveletdwt.IDWT1D( coeffOut, L, reconstructArray );
  std::cout << "Inverse Wavelet Transform: result signal length = " << 
      reconstructArray.GetNumberOfValues() << std::endl;
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    std::cout << reconstructArray.GetPortalConstControl().Get(i) << std::endl;
  }
}



void DebugRectangleCopy()
{
  vtkm::Id sigX = 12;
  vtkm::Id sigY = 12;  
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>( i ) );
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  // make output array
  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;
  
  // make bookkeeping array
  std::vector<vtkm::Id> L; 
  
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF5_3;
  vtkm::worklet::WaveletCompressor wavelet( wname );
  wavelet.WaveDecompose2D( inputArray, 2, sigX, sigY, outputArray, L, 
                           VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  for( vtkm::Id i = 0; i < outputArray.GetNumberOfValues(); i++ )
  {
    std::cout << std::setw( 10 );
    std::cout << outputArray.GetPortalConstControl().Get(i) << "\t";
    if( i % sigX == sigX-1 )   
      std::cout << std::endl;
  }
  std::cout << std::endl;
}

void DebugDWTIDWT2D()
{
  vtkm::Id sigX = 20;
  vtkm::Id sigY = 20;  
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>( i ) );
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> coeffOut;
  std::vector<vtkm::Id> L;
  vtkm::Id nlevels = 2;

  // Forward Transform
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF9_7;
  vtkm::worklet::WaveletCompressor wavelet( wname );
  wavelet.WaveDecompose2D( inputArray, nlevels, sigX, sigY, coeffOut, L,
                           VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

/*
  for( vtkm::Id i = 0; i < coeffOut.GetNumberOfValues(); i++ )
  {
    std::cout << std::setw( 10 );
    std::cout << coeffOut.GetPortalConstControl().Get(i) << "\t";
    if( i % sigX == sigX-1 )   
      std::cout << std::endl;
  }
  std::cout << std::endl;
*/ 

  // Inverse Transform
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  wavelet.WaveReconstruct2D( coeffOut, nlevels, sigX, sigY, reconstructArray, L,
                             VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    std::cout << reconstructArray.GetPortalConstControl().Get(i) << "  ";
    if( i % sigX == sigX-1 )   
      std::cout << std::endl;
  }
}


void DebugWaveDecomposeReconstruct()
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Default test size is 20. " << std::endl;
  std::cout << "Input a new size to test." << std::endl;
  std::cout << "Input 0 to stick with 20." << std::endl;
  vtkm::Id tmpIn;
  std::cin >> tmpIn;
  if( tmpIn != 0 )
    sigLen = tmpIn;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>(i) );
    //tmpVector.push_back( 100.0 * vtkm::Sin(static_cast<vtkm::Float64>(i)/100.0 ));
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::Id nLevels = 2;
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF8_4;
  vtkm::worklet::WaveletCompressor compressor( wname );

  // User input of decompose levels
  vtkm::Id maxLevel = compressor.GetWaveletMaxLevel( sigLen );
  std::cout << "Input how many wavelet transform levels to perform, between 1 and "
            << maxLevel << std::endl;
  vtkm::Id levTemp;
  std::cin >> levTemp;
  if( levTemp > 0 && levTemp <= maxLevel )
    nLevels = levTemp;
  else
  {
    std::cerr << "not valid levels of transforms" << std::endl;
    exit(1);
  }
  std::cout << "Input a compression ratio ( >=1 )to test. "
            << "1 means no compression. " << std::endl;
  vtkm::Float64 cratio;
  std::cin >> cratio;
  VTKM_ASSERT ( cratio >= 1 );

  std::vector<vtkm::Id> L;

  // Decompose
  vtkm::cont::Timer<> timer;
  compressor.WaveDecompose( inputArray, nLevels, outputArray, L, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cout << "Decompose time         = " << elapsedTime << std::endl;
  
  // Squash small coefficients
  timer.Reset();
  compressor.SquashCoefficients( outputArray, cratio, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Thresholding time      = " << elapsedTime << std::endl;
  


  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Reset();
  compressor.WaveReconstruct( outputArray, nLevels, L, reconstructArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction( inputArray, reconstructArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  timer.Reset();
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    VTKM_TEST_ASSERT( test_equal( reconstructArray.GetPortalConstControl().Get(i),
                                  100.0 * vtkm::Sin( static_cast<vtkm::Float64>(i)/100.0 )), 
                                  "output value not the same..." );
  }
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Verification time      = " << elapsedTime << std::endl;

}


VTKM_CONT_EXPORT
void TestWaveDecomposeReconstruct()
{
  std::cout << "Testing WaveletCompressor on a 2 million sized array " << std::endl;
  vtkm::Id million = 1000000;
  vtkm::Id sigLen = million * 2;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
  {
    tmpVector.push_back( 100.0 * vtkm::Sin(static_cast<vtkm::Float64>(i)/100.0 ));
  }
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF8_4;
  vtkm::worklet::WaveletCompressor compressor( wname );

  // User maximum decompose levels, and no compression
  vtkm::Id maxLevel = compressor.GetWaveletMaxLevel( sigLen );
  vtkm::Id nLevels = maxLevel;

  std::vector<vtkm::Id> L;

  // Decompose
  vtkm::cont::Timer<> timer;
  compressor.WaveDecompose( inputArray, nLevels, outputArray, L, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cout << "Decompose time         = " << elapsedTime << std::endl;
  
  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Reset();
  compressor.WaveReconstruct( outputArray, nLevels, L, reconstructArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction( inputArray, reconstructArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  timer.Reset();
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    VTKM_TEST_ASSERT( test_equal( reconstructArray.GetPortalConstControl().Get(i),
                                  100.0 * vtkm::Sin( static_cast<vtkm::Float64>(i)/100.0 )), 
                                  "WaveletCompressor worklet failed..." );
  }
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Verification time      = " << elapsedTime << std::endl;

}

void TestWaveletCompressor()
{
  //DebugDWTIDWT1D();
  //DebugWaveDecomposeReconstruct();
  DebugDWTIDWT2D();
  //DebugRectangleCopy();
  //TestWaveDecomposeReconstruct();
}

int UnitTestWaveletCompressor(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor);
}
