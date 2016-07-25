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

//#include <vtkm/filter/internal/WaveletDWT.h>
#include <vtkm/filter/WaveletCompressor.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vector>



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

  vtkm::filter::internal::WaveletDWT w("CDF9/7");
  /*
  typedef vtkm::cont::ArrayHandleConcatenate< ArrayType, ArrayType> 
            ArrayConcat;
  typedef vtkm::cont::ArrayHandleConcatenate< ArrayConcat, ArrayType > ArrayConcat2;
  ArrayConcat2 outputArray;
  */
  w.Extend1D( inputArray, outputArray, 4, 
      vtkm::filter::internal::SYMW, vtkm::filter::internal::SYMW );

  std::cout << "Start testing Extend1D" << std::endl;
  for (vtkm::Id i = 0; i < outputArray.GetNumberOfValues(); ++i)
      std::cout << outputArray.GetPortalConstControl().Get(i) << std::endl;
  std::cout << "\nFinish testing Extend1D" << std::endl;
}

void TestDWTIDWT1D()
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Default test size is 20. " << std::endl;
  std::cout << "Input a new size to test." << std::endl;
  std::cout << "Input 0 to stick with 20." << std::endl;
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
  vtkm::filter::internal::WaveletDWT waveletdwt( "CDF5/3" );
  waveletdwt.DWT1D( inputArray, coeffOut, L );

  std::cout << "Forward Wavelet Transform: result coeff length = " << 
      coeffOut.GetNumberOfValues() << std::endl;
  printf("L[0] = %lld, L[1] = %lld, L[2] = %lld\n", L[0], L[1], L[2] );
  for( vtkm::Id i; i < coeffOut.GetNumberOfValues(); i++ )
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
  for( vtkm::Id i; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    std::cout << reconstructArray.GetPortalConstControl().Get(i) << std::endl;
  }
}

void TestWaveDecompose()
{
  vtkm::Id sigLen = 20;
  std::cout << "Testing Wavelets Worklet" << std::endl;
  std::cout << "Default test size is 20. " << std::endl;
  std::cout << "Input a new size to test." << std::endl;
  std::cout << "Input 0 to stick with 20." << std::endl;
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

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::filter::WaveletCompressor compressor("CDF9/7");
  compressor.WaveDecompose( inputArray, 1, outputArray );
  
  std::cout << "Output array has length = " << 
      outputArray.GetNumberOfValues() << std::endl;
  for( vtkm::Id i; i < outputArray.GetNumberOfValues(); i++ )
  {
    std::cout << outputArray.GetPortalConstControl().Get(i) << std::endl;
  }
}

void TestWaveletCompressor()
{
  std::cout << "Welcome to WaveletCompressorFilter test program :) " << std::endl;
  TestExtend1D();
  //TestDWTIDWT1D();
  //TestWaveDecompose();
}

int UnitTestWaveletCompressorFilter(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor);
}
