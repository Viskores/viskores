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
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Input a size to test." << std::endl;
  vtkm::Id tmpIn;
  vtkm::Id million = 1000000;
  std::cin >> tmpIn;
  if( tmpIn != 0 )
    sigLen = tmpIn * million;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>( i ) );
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> coeffOut;
  std::vector<vtkm::Id> L(3, 0);

  // Forward Transform
  vtkm::worklet::wavelets::WaveletDWT waveletdwt( "CDF9/7" );
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
  vtkm::Id sigX = 5;
  vtkm::Id sigY = 7;  
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>( i ) );
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  // make a big rectangle of zeros
  vtkm::Id bigX = 15;
  vtkm::Id bigY = 17;
  std::vector<vtkm::Float64> tmpVector2( bigX * bigY, 0 );
  vtkm::cont::ArrayHandle<vtkm::Float64> bigArray = 
    vtkm::cont::make_ArrayHandle(tmpVector2);
  
  // test copy to
  vtkm::Id xStart = 9;
  vtkm::Id yStart = 5;
  typedef vtkm::worklet::wavelets::RectangleCopyTo  CopyToWorklet;
  CopyToWorklet cp( sigX, sigY, bigX, bigY, xStart, yStart );
  vtkm::worklet::DispatcherMapField< CopyToWorklet > dispatcher( cp  );
  dispatcher.Invoke(inputArray, bigArray);

  // test copy from
  vtkm::cont::ArrayHandle<vtkm::Float64> copyFromArray;
  copyFromArray.Allocate( sigX * sigY );
  typedef vtkm::worklet::wavelets::RectangleCopyFrom  CopyFromWorklet;
  CopyFromWorklet cpFrom( sigX, sigY, bigX, bigY, xStart, yStart );
  vtkm::worklet::DispatcherMapField< CopyFromWorklet > dispatcherFrom( cpFrom );
  dispatcherFrom.Invoke( copyFromArray, bigArray);
  

  for( vtkm::Id i = 0; i < copyFromArray.GetNumberOfValues(); i++ )
  {
    std::cout << std::setw( 5 );
    std::cout << copyFromArray.GetPortalConstControl().Get(i) << "\t";
    if( i % sigX == sigX-1 )   
      std::cout << std::endl;
  }
  std::cout << std::endl;
}

void DebugDWTIDWT2D()
{
  vtkm::Id sigX = 9;
  vtkm::Id sigY = 11;  
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<vtkm::Float64>( i ) );
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> coeffOut;
  std::vector<vtkm::Id> L(10, 0);

  // Forward Transform
  vtkm::worklet::wavelets::WaveletDWT waveletdwt( "CDF9/7" );
  waveletdwt.DWT2D( inputArray, sigX, sigY, coeffOut, L );

  for( vtkm::Id i = 0; i < coeffOut.GetNumberOfValues(); i++ )
  {
    std::cout << std::setw( 10 );
    std::cout << coeffOut.GetPortalConstControl().Get(i) << "\t";
    if( i % sigX == sigX-1 )   
      std::cout << std::endl;
  }
  std::cout << std::endl;
  

  // Inverse Transform
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  waveletdwt.IDWT2D( coeffOut, L, reconstructArray );
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    std::cout << reconstructArray.GetPortalConstControl().Get(i) << std::endl;
  }
}


VTKM_CONT_EXPORT
void DebugWaveDecomposeReconstruct()
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Default test size is 20. " << std::endl;
  std::cout << "Input a new size to test (in millions)." << std::endl;
  std::cout << "Input 0 to stick with 20." << std::endl;
  vtkm::Id tmpIn;
  vtkm::Id million   = 1000000;
  vtkm::Id thousand  = 1000;
  //std::cin >> tmpIn;
  tmpIn = 100;
  if( tmpIn != 0 )
    sigLen = tmpIn * million;

  // make input data array handle
  std::vector<vtkm::Float64> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( 100.0 * vtkm::Sin(static_cast<vtkm::Float64>(i)/100.0 ));
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::Id nLevels = 2;
  vtkm::worklet::WaveletCompressor compressor("CDF9/7");

  // User input of decompose levels
  vtkm::Id maxLevel = compressor.GetWaveletMaxLevel( sigLen );
  std::cout << "Input how many wavelet transform levels to perform, between 1 and "
            << maxLevel << std::endl;
  vtkm::Id levTemp;
  //std::cin >> levTemp;
  levTemp = 17;
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
  //std::cin >> cratio;
  cratio = 10;
  VTKM_ASSERT ( cratio >= 1 );

  std::vector<vtkm::Id> L;

  // Decompose
  vtkm::cont::Timer<> timer;
  compressor.WaveDecompose( inputArray, nLevels, outputArray, L );

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cout << "Decompose time         = " << elapsedTime << std::endl;
  

  // Squash small coefficients
  timer.Reset();
  compressor.SquashCoefficients( outputArray, cratio );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Thresholding time      = " << elapsedTime << std::endl;


  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Reset();
  compressor.WaveReconstruct( outputArray, nLevels, L, reconstructArray );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction( inputArray, reconstructArray );

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
    tmpVector.push_back( 100.0 * vtkm::Sin(static_cast<vtkm::Float64>(i)/100.0 ));
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::WaveletCompressor compressor("CDF9/7");

  // User maximum decompose levels, and no compression
  vtkm::Id maxLevel = compressor.GetWaveletMaxLevel( sigLen );
  vtkm::Id nLevels = maxLevel;

  std::vector<vtkm::Id> L;

  // Decompose
  vtkm::cont::Timer<> timer;
  compressor.WaveDecompose( inputArray, nLevels, outputArray, L );

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cout << "Decompose time         = " << elapsedTime << std::endl;
  
  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Reset();
  compressor.WaveReconstruct( outputArray, nLevels, L, reconstructArray );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction( inputArray, reconstructArray );

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
  //DebugDWTIDWT2D();
  DebugRectangleCopy();
  //TestWaveDecomposeReconstruct();
}

int UnitTestWaveletCompressor(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor);
}
