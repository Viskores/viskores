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

#include <vtkm/cont/ArrayHandleInterpreter.h>
#include <vtkm/cont/ArrayHandleConcatenate2DTopDown.h>
#include <vtkm/cont/ArrayHandleConcatenate2DLeftRight.h>

#include <vector>
#include <iomanip>

namespace vtkm
{
namespace worklet
{
namespace wavelets
{
class SineWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldInOut<>);
  typedef void ExecutionSignature(_1, WorkIndex);

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(T& x, const vtkm::Id& workIdx) const 
  {
    x = vtkm::Sin(vtkm::Float64(workIdx) / 100.0) * 100.0;
  }
};
}
}
}

template< typename ArrayType >
void FillArray( ArrayType& array )
{
  typedef vtkm::worklet::wavelets::SineWorklet SineWorklet;
  SineWorklet worklet;
  vtkm::worklet::DispatcherMapField< SineWorklet > dispatcher( worklet );
  dispatcher.Invoke( array );
}

void Debug2DExtend()
{
  vtkm::Id NX = 5;
  vtkm::Id NY = 4;
  typedef vtkm::cont::ArrayHandleInterpreter< vtkm::Id >   ArrayInterp;
  ArrayInterp     left, center, right;
  left.PrepareForOutput( NX * NY, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  left.InterpretAs2D( NX, NY );
  right.PrepareForOutput( NX * NY, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  right.InterpretAs2D( NX, NY );
  center.PrepareForOutput( 2*NX * NY, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  center.InterpretAs2D( 2*NX, NY );
  for( vtkm::Id i = 0; i < 2*NX*NY; i++ )
    center.GetPortalControl().Set(i, i);

  typedef vtkm::cont::ArrayHandleConcatenate2DLeftRight< ArrayInterp, ArrayInterp >
          ConcatLeftOn;
  typedef vtkm::cont::ArrayHandleConcatenate2DLeftRight< ConcatLeftOn, ArrayInterp >
          ConcatRightOn;
  ConcatRightOn output;

  vtkm::worklet::wavelets::WaveletDWT dwt( vtkm::worklet::wavelets::CDF9_7 );
  vtkm::worklet::wavelets::DWTMode leftMode   = vtkm::worklet::wavelets::SYMH; 
  vtkm::worklet::wavelets::DWTMode rightMode  = vtkm::worklet::wavelets::SYMH; 
  dwt.Extend2D( center, output, 4, leftMode, rightMode, false, false, 
      VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  for(   vtkm::Id j = 0; j < output.GetDimY(); j++ )
  {
    for( vtkm::Id i = 0; i < output.GetDimX(); i++ )
      std::cout << output.Get2D( i, j ) << " \t";
    std::cout << std::endl;
  }
}

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
  waveletdwt.DWT1D( inputArray, coeffOut, L, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

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
  waveletdwt.IDWT1D( coeffOut, L, reconstructArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
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


void TestDecomposeReconstruct2D()
{
  vtkm::Id sigX = 1000;
  vtkm::Id sigY = 2000;  
  std::cout << "Please input X to test a X^2 square: " << std::endl;
  std::cin >> sigX;
  sigY = sigX;
  //std::cout << "Testing wavelet compressor on 1000x2000 rectangle" << std::endl;
  vtkm::Id sigLen = sigX * sigY;

  // make input data array handle
  vtkm::cont::ArrayHandle<vtkm::Float64> inputArray;
  inputArray.PrepareForOutput( sigLen, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  FillArray( inputArray );

  vtkm::cont::ArrayHandle<vtkm::Float64> outputArray;

  // Use a WaveletCompressor
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::CDF9_7;
  vtkm::worklet::WaveletCompressor compressor( wname );

  vtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel( sigX );
  vtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel( sigY );
  vtkm::Id nLevels   = vtkm::Min( XMaxLevel, YMaxLevel );
  //nLevels = 1;
  std::vector<vtkm::Id> L;

  // Decompose
  vtkm::cont::Timer<> timer;
  vtkm::Float64 computationTime = 
  compressor.WaveDecompose2D( inputArray, nLevels, sigX, sigY, outputArray, L, 
                              VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  vtkm::Float64 elapsedTime = timer.GetElapsedTime();  
  std::cout << "Decompose time         = " << elapsedTime << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;

  // Squash small coefficients
  timer.Reset();
  vtkm::Float64 cratio = 1.0;
  compressor.SquashCoefficients( outputArray, cratio, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Squash time            = " << elapsedTime << std::endl;

  // Reconstruct
  vtkm::cont::ArrayHandle<vtkm::Float64> reconstructArray;
  timer.Reset();
  computationTime = 
  compressor.WaveReconstruct2D( outputArray, nLevels, sigX, sigY, reconstructArray, L,
                                VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Reconstruction time    = " << elapsedTime << std::endl;
  std::cout << "  ->computation time   = " << computationTime << std::endl;

  compressor.EvaluateReconstruction( inputArray, reconstructArray, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  timer.Reset();
  for( vtkm::Id i = 0; i < reconstructArray.GetNumberOfValues(); i++ )
  {
    VTKM_TEST_ASSERT( test_equal( reconstructArray.GetPortalConstControl().Get(i),
                                  inputArray.GetPortalConstControl().Get(i) ),
                                  "output value not the same..." );
  }
  elapsedTime = timer.GetElapsedTime();  
  std::cout << "Verification time      = " << elapsedTime << std::endl;
}


void TestDecomposeReconstruct1D()
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
  //DebugRectangleCopy();
  //TestDecomposeReconstruct1D();
  //TestDecomposeReconstruct2D();
  Debug2DExtend();
}

int UnitTestWaveletCompressor(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor);
}
