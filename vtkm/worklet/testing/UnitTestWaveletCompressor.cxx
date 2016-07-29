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


VTKM_CONT_EXPORT
void TestExtend1D()
{
  // make input data array handle
  typedef vtkm::Float64 T;
  typedef vtkm::cont::ArrayHandle<T>     ArrayType;
  vtkm::Id sigLen = 20;
  std::vector<T> tmpVector;
  for( vtkm::Id i = 0; i < sigLen; i++ )
    tmpVector.push_back( static_cast<T>(i) );
 
  vtkm::cont::ArrayHandle<T> inputArray = 
    vtkm::cont::make_ArrayHandle(tmpVector);
  ArrayType outputArray;

  vtkm::worklet::wavelets::WaveletDWT w("CDF9/7");

  w.Extend1D( inputArray, outputArray, 4, 
      vtkm::worklet::wavelets::SYMW, vtkm::worklet::wavelets::SYMW );


  std::cout << "Start testing Extend1D" << std::endl;
  for (vtkm::Id i = 0; i < outputArray.GetNumberOfValues(); ++i)
      std::cout << outputArray.GetPortalConstControl().Get(i) << std::endl;
  std::cout << "\nFinish testing Extend1D" << std::endl;
}

VTKM_CONT_EXPORT
void TestDWTIDWT1D()
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Input a size to test." << std::endl;
  vtkm::Id tmpIn;
  vtkm::Id million = 1;//1000000;
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
  vtkm::Id L[3];

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

VTKM_CONT_EXPORT
void TestWaveDecomposeReconstruct()
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Default test size is 20. " << std::endl;
  std::cout << "Input a new size to test." << std::endl;
  std::cout << "Input 0 to stick with 20." << std::endl;
  vtkm::Id tmpIn;
  vtkm::Id million = 1000000;
  std::cin >> tmpIn;
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
  std::cout << "Please input how many wavelet transform levels to perform, between 1 and "
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

  vtkm::Id* L = new vtkm::Id[ nLevels + 2 ];

  // Use a timer and decompose
  vtkm::cont::Timer<> timer;
  compressor.WaveDecompose( inputArray, nLevels, outputArray, L );

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cout << "Decompose takes time: " << elapsedTime << std::endl;
  

  // Squash small coefficients
  std::cout << "Input a compression ratio ( >=1 )to test. " << std::endl;
  std::cout << "1 means no compression, only forward and inverse wavelet transform. " << std::endl;
  vtkm::Id cratio;
  std::cin >> cratio;
  VTKM_ASSERT ( cratio >= 1 );
  compressor.SquashCoefficients( outputArray, cratio );
  /*
  std::cout << "Coefficients after squash: " << std::endl;
  for( vtkm::Id i = 0; i < outputArray.GetNumberOfValues(); i++ )
    std::cout << outputArray.GetPortalConstControl().Get(i) << std::endl; 
   */


  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Reset();
  compressor.WaveReconstruct( outputArray, nLevels, L, reconstructArray );

  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Reconstruction takes time: " << elapsedTime << std::endl;

  compressor.EvaluateReconstruction( inputArray, reconstructArray );

  timer.Reset();
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    VTKM_TEST_ASSERT( test_equal( reconstructArray.GetPortalConstControl().Get(i),
                                  100.0 * vtkm::Sin( static_cast<vtkm::Float64>(i)/100.0 )), 
                                  "output value not the same..." );
  }
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Verification takes time: " << elapsedTime << std::endl;

  delete[] L;

}

void TestWaveletCompressor()
{
  std::cout << "Welcome to WaveletCompressor test program :) " << std::endl;
  //TestExtend1D();
  //TestDWTIDWT1D();
  TestWaveDecomposeReconstruct();
}

int UnitTestWaveletCompressor(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor);
}
