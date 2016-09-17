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

void DebugExtend2D()
{
  vtkm::Id NX = 10;
  vtkm::Id NY = 10;
  vtkm::Id addLen = 4;
  typedef vtkm::cont::ArrayHandle< vtkm::Float64 >   ArrayType;
  ArrayType     left1, left2, center, right1, right2;
  ArrayType     centerExtended;
  
  center.PrepareForOutput( NX * NY, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  for( vtkm::Id i = 0; i < NX*NY; i++ )
    center.GetPortalControl().Set(i, i);

  typedef vtkm::worklet::wavelets::ExtensionWorklet2D       ExtWorklet;
  typedef vtkm::worklet::wavelets::LeftExtensionWorklet2D   LeftExtWorklet;
  typedef vtkm::worklet::wavelets::RightExtensionWorklet2D  RightExtWorklet;

  vtkm::worklet::wavelets::ExtensionDirection2D 
      extdirLeft = vtkm::worklet::wavelets::ExtensionDirection2D::LEFT;
  vtkm::worklet::wavelets::ExtensionDirection2D 
      extdirRight = vtkm::worklet::wavelets::ExtensionDirection2D::RIGHT;
  vtkm::worklet::wavelets::DWTMode mode = vtkm::worklet::wavelets::SYMW;

  vtkm::worklet::wavelets::WaveletDWT dwt( vtkm::worklet::wavelets::CDF9_7 );

  // compute real values
  {
    dwt.Extend2D( center, NX, NY, centerExtended, addLen, mode, mode, false, true,
                  VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  }
  // compute test implementation
  {
    dwt.Extend2Dv3( center, NX, NY, left1, right1, addLen, mode, mode, false, true, true,
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  }
}


void DebugDWT2D()
{
  vtkm::Id NX = 6;
  vtkm::Id NY = 5;
  typedef vtkm::cont::ArrayHandle< vtkm::Float64 >   ArrayType;
  ArrayType     left, center, right;
  
  center.PrepareForOutput( NX * NY, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  for( vtkm::Id i = 0; i < NX*NY; i++ )
    center.GetPortalControl().Set(i, i);

  ArrayType output1, output2, output3;
  std::vector<vtkm::Id> L(10, 0);

  vtkm::worklet::wavelets::WaveletDWT dwt( vtkm::worklet::wavelets::HAAR );

  // get true results
  dwt.DWT2Dv2(center, NX, NY, output1, L, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

  // get test results
  dwt.DWT2Dv3( center, NX, NY, output3, L, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );

  for( vtkm::Id i = 0; i < output1.GetNumberOfValues(); i++ )
  {
    VTKM_TEST_ASSERT( test_equal( output1.GetPortalConstControl().Get(i),
                                  output3.GetPortalConstControl().Get(i)),
                                  "WaveletCompressor worklet failed..." );
  }
  
  dwt.Print2DArray("\ntrue results after 2D DWT:", output1, NX );
  dwt.Print2DArray("\ntest results after 2D DWT:", output3, NX ); 

ArrayType   idwt_out1, idwt_out2;
  
  // true results go through IDWT
  dwt.IDWT2Dv2( output1, L, idwt_out1, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  
  // test results go through IDWT
  dwt.IDWT2Dv3( output3, L, idwt_out2, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
std::cerr << "finish IDWT2Dv3" << std::endl;

std::cout << "\ntrue results after IDWT:" << std::endl;
  for( vtkm::Id i = 0; i < idwt_out1.GetNumberOfValues(); i++ )
  {
    std::cout << idwt_out1.GetPortalConstControl().Get(i) << "  ";
    if( i % NX == NX - 1 )
      std::cout << std::endl;
  }
  
std::cout << "\ntest results after IDWT:" << std::endl;
  for( vtkm::Id i = 0; i < idwt_out2.GetNumberOfValues(); i++ )
  {
    std::cout << idwt_out2.GetPortalConstControl().Get(i) << "  ";
    if( i % NX == NX - 1 )
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
  waveletdwt.IDWT1D( coeffOut, L, reconstructArray, false, VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
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
  vtkm::worklet::wavelets::WaveletName wname = vtkm::worklet::wavelets::HAAR;
  vtkm::worklet::WaveletCompressor compressor( wname );

  vtkm::Id XMaxLevel = compressor.GetWaveletMaxLevel( sigX );
  vtkm::Id YMaxLevel = compressor.GetWaveletMaxLevel( sigY );
  vtkm::Id nLevels   = vtkm::Min( XMaxLevel, YMaxLevel );
  //nLevels = 1;
  std::cout << "Decomposition levels   = " << nLevels << std::endl;
  std::vector<vtkm::Id> L;
  vtkm::Float64 computationTime = 0.0;
  vtkm::Float64 elapsedTime = 0.0;

  // Decompose
  vtkm::cont::Timer<> timer;
  computationTime = 
  compressor.WaveDecompose2D( inputArray, nLevels, sigX, sigY, outputArray, L, 
                              VTKM_DEFAULT_DEVICE_ADAPTER_TAG() );
  elapsedTime = timer.GetElapsedTime();  
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
  DebugDWT2D();
  //DebugExtend2D();
}

int UnitTestWaveletCompressor(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWaveletCompressor);
}
