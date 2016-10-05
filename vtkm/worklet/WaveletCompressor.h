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

#ifndef vtk_m_worklet_waveletcompressor_h
#define vtk_m_worklet_waveletcompressor_h

#include <vtkm/worklet/wavelets/WaveletDWT.h>

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {

class WaveletCompressor : public vtkm::worklet::wavelets::WaveletDWT
{
public:

  // Constructor
  WaveletCompressor( wavelets::WaveletName name ) : WaveletDWT( name ) {} 


  // Multi-level 1D wavelet decomposition
  template< typename SignalArrayType, typename CoeffArrayType, typename DeviceTag >
  VTKM_CONT_EXPORT
  vtkm::Id WaveDecompose( const SignalArrayType      &sigIn,   // Input
                                vtkm::Id             nLevels,  // n levels of DWT
                                CoeffArrayType       &coeffOut,
                                std::vector<vtkm::Id> &L,
                                DeviceTag                     )
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if( nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel( sigInLen ) )
    {
      throw vtkm::cont::ErrorControlBadValue("Number of levels of transform is not supported! ");
    }
    if( nLevels == 0 )  //  0 levels means no transform
    {
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( sigIn, coeffOut );
      return 0;
    }

    this->ComputeL( sigInLen, nLevels, L ); // memory for L is allocated by ComputeL().
    vtkm::Id CLength = this->ComputeCoeffLength( L, nLevels );
    VTKM_ASSERT( CLength == sigInLen );

    vtkm::Id sigInPtr = 0;  // pseudo pointer for the beginning of input array 
    vtkm::Id len = sigInLen;
    vtkm::Id cALen = WaveletBase::GetApproxLength( len );
    vtkm::Id cptr;          // pseudo pointer for the beginning of output array
    vtkm::Id tlen = 0;
    std::vector<vtkm::Id> L1d(3, 0);

    // Use an intermediate array
    typedef typename CoeffArrayType::ValueType          OutputValueType;
    typedef vtkm::cont::ArrayHandle< OutputValueType >  InterArrayType;

    // Define a few more types
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, CoeffArrayType > 
              PermutArrayType;

    vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( sigIn, coeffOut );

    for( vtkm::Id i = nLevels; i > 0; i-- )
    {
      tlen += L[ size_t(i) ];
      cptr = 0 + CLength - tlen - cALen;
      
      // make input array (permutation array)
      IdArrayType       inputIndices( sigInPtr, 1, len );
      PermutArrayType   input( inputIndices, coeffOut ); 
      // make output array 
      InterArrayType    output;

      WaveletDWT::DWT1D( input, output, L1d, DeviceTag() );

      // move intermediate results to final array
      WaveletBase::DeviceCopyStartX( output, coeffOut, cptr, DeviceTag() );

      // update pseudo pointers
      len = cALen;
      cALen = WaveletBase::GetApproxLength( cALen );
      sigInPtr = cptr;
    }

    return 0;
  }


  // Multi-level 1D wavelet reconstruction
  template< typename CoeffArrayType, typename SignalArrayType, typename DeviceTag >
  VTKM_CONT_EXPORT
  vtkm::Id WaveReconstruct( const CoeffArrayType     &coeffIn,   // Input
                                  vtkm::Id           nLevels,    // n levels of DWT
                                  std::vector<vtkm::Id> &L,
                                  SignalArrayType    &sigOut,
                                  DeviceTag                  )
  {
    VTKM_ASSERT( nLevels > 0 );
    vtkm::Id LLength = nLevels + 2;
    VTKM_ASSERT( vtkm::Id(L.size()) == LLength );

    //vtkm::Id L1d[3] = {L[0], L[1], 0};
    std::vector<vtkm::Id> L1d(3, 0);  // three elements
    L1d[0] = L[0];
    L1d[1] = L[1];

    typedef typename SignalArrayType::ValueType              OutValueType;
    typedef vtkm::cont::ArrayHandle< OutValueType >          OutArrayBasic;
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, SignalArrayType > 
                  PermutArrayType;

    vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( coeffIn, sigOut );

    for( vtkm::Id i = 1; i <= nLevels; i++ )
    {
      L1d[2] = this->GetApproxLengthLevN( L[ size_t(LLength-1) ], nLevels-i );

      // Make an input array
      IdArrayType inputIndices( 0, 1, L1d[2] );
      PermutArrayType input( inputIndices, sigOut ); 
      
      // Make an output array
      OutArrayBasic output;
      
      WaveletDWT::IDWT1D( input, L1d, output, false, DeviceTag() );
      VTKM_ASSERT( output.GetNumberOfValues() == L1d[2] );

      // Move output to intermediate array
      WaveletBase::DeviceCopyStartX( output, sigOut, 0, DeviceTag() );

      L1d[0] = L1d[2];
      L1d[1] = L[ size_t(i+1) ];
    }

    return 0;
  }


  // Multi-level 2D wavelet decomposition
  template< typename InArrayType, typename OutArrayType, typename DeviceTag>
  VTKM_CONT_EXPORT
  FLOAT_64 WaveDecompose2D( const InArrayType           &sigIn,   // Input
                                  vtkm::Id              nLevels,  // n levels of DWT
                                  vtkm::Id              inX,      // Input X dim
                                  vtkm::Id              inY,      // Input Y dim
                                  OutArrayType          &coeffOut,
                                  std::vector<vtkm::Id> &L,
                                  DeviceTag                       )
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    VTKM_ASSERT( inX * inY == sigInLen );
    if( nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel( inX ) ||
                       nLevels > WaveletBase::GetWaveletMaxLevel( inY ) )
    {
      throw vtkm::cont::ErrorControlBadValue("Number of levels of transform is not supported! ");
    }
    if( nLevels == 0 )  //  0 levels means no transform
    {
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( sigIn, coeffOut );
      return 0;
    }

    this->ComputeL2( inX, inY, nLevels, L );
    vtkm::Id CLength = this->ComputeCoeffLength2( L, nLevels );
    VTKM_ASSERT( CLength == sigInLen );

    vtkm::Id currentLenX     = inX;
    vtkm::Id currentLenY     = inY;
    std::vector<vtkm::Id> L2d(10, 0);
    vtkm::Float64 computationTime = 0.0;

    typedef typename OutArrayType::ValueType          OutValueType;
    typedef vtkm::cont::ArrayHandle<OutValueType>     OutBasicArray;

    // First level transform operates writes to the output array
    computationTime += WaveletDWT::DWT2Dv3( sigIn, 
                                            currentLenX,       currentLenY, 
                                            0,                 0,
                                            currentLenX,       currentLenY, 
                                            coeffOut, L2d, DeviceTag() );
    VTKM_ASSERT( coeffOut.GetNumberOfValues() == currentLenX * currentLenY );
    currentLenX = WaveletBase::GetApproxLength( currentLenX );
    currentLenY = WaveletBase::GetApproxLength( currentLenY );

    // Successor transforms writes to a temporary array
    for( vtkm::Id i = nLevels-1; i > 0; i-- )
    {
      //make temporary output array
      OutBasicArray tempOutput;

      computationTime +=
      WaveletDWT::DWT2Dv3(  coeffOut, 
                            inX,              inY, 
                            0,                0,
                            currentLenX,      currentLenY, 
                            tempOutput, L2d, DeviceTag() );

      // copy results to coeffOut
      WaveletBase::DeviceRectangleCopyTo( tempOutput, currentLenX, currentLenY,
                                          coeffOut, inX, inY, 0, 0, DeviceTag() );

      // update currentLen
      currentLenX = WaveletBase::GetApproxLength( currentLenX );
      currentLenY = WaveletBase::GetApproxLength( currentLenY );
    }

    return computationTime;
  }


  // Multi-level 2D wavelet reconstruction
  template< typename InArrayType, typename OutArrayType, typename DeviceTag>
  VTKM_CONT_EXPORT
  FLOAT_64 WaveReconstruct2D( const InArrayType           &arrIn,   // Input
                                    vtkm::Id              nLevels,  // n levels of DWT
                                    vtkm::Id              inX,      // Input X dim
                                    vtkm::Id              inY,      // Input Y dim
                                    OutArrayType          &arrOut,
                                    std::vector<vtkm::Id> &L,
                                    DeviceTag                       )
  {
    vtkm::Id arrInLen = arrIn.GetNumberOfValues();
    VTKM_ASSERT( inX * inY == arrInLen );
    if( nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel( inX ) ||
                       nLevels > WaveletBase::GetWaveletMaxLevel( inY ) )
    {
      throw vtkm::cont::ErrorControlBadValue("Number of levels of transform is not supported! ");
    }
    typedef typename OutArrayType::ValueType          OutValueType;
    typedef vtkm::cont::ArrayHandle<OutValueType>     OutBasicArray;
    OutBasicArray                                     outBuffer;
    vtkm::Float64 computationTime = 0.0;
  
    if( nLevels == 0 )  //  0 levels means no transform
    {
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( arrIn, arrOut );
      return 0;
    }
    else
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( arrIn, outBuffer );
      
    VTKM_ASSERT( vtkm::Id(L.size()) == 6 * nLevels + 4 );

    std::vector<vtkm::Id> L2d(10, 0);
    L2d[0]  =   L[0];   
    L2d[1]  =   L[1];   
    L2d[2]  =   L[2];   
    L2d[3]  =   L[3];   
    L2d[4]  =   L[4];   
    L2d[5]  =   L[5];   
    L2d[6]  =   L[6];   
    L2d[7]  =   L[7];   
    
    // All transforms but the last operate on temporary arrays
    for( size_t i = 1; i < static_cast<size_t>(nLevels); i++ )
    {
      L2d[8] = L2d[0] + L2d[4];     // This is always true for Biorthogonal wavelets
      L2d[9] = L2d[1] + L2d[3];     // (same above)

      // make input, output array
      OutBasicArray tempInput, tempOutput;
      WaveletBase::DeviceRectangleCopyFrom( tempInput, L2d[8], L2d[9],
                                            outBuffer,  inX, inY, 0, 0, DeviceTag() );

      // IDWT
      computationTime +=
      WaveletDWT::IDWT2Dv3( tempInput, L2d, tempOutput, DeviceTag() );

      // copy back reconstructed block
      WaveletBase::DeviceRectangleCopyTo( tempOutput, L2d[8], L2d[9],
                                          outBuffer, inX, inY, 0, 0, DeviceTag() );
    
      // update L2d array
      L2d[0] =  L2d[8];
      L2d[1] =  L2d[9];
      L2d[2] = L[6*i+2];
      L2d[3] = L[6*i+3];
      L2d[4] = L[6*i+4];
      L2d[5] = L[6*i+5];
      L2d[6] = L[6*i+6];
      L2d[7] = L[6*i+7];
    }

    // The last transform takes place in place 
    L2d[8] = L2d[0] + L2d[4];
    L2d[9] = L2d[1] + L2d[3];
    computationTime += WaveletDWT::IDWT2Dv3( outBuffer, L2d, arrOut, DeviceTag() );

    return computationTime;    
  }


  // Squash coefficients smaller than a threshold
  template< typename CoeffArrayType, typename DeviceTag >
  vtkm::Id SquashCoefficients( CoeffArrayType   &coeffIn,
                               vtkm::Float64    ratio,
                               DeviceTag                )
  {
    if( ratio > 1 )
    {
      vtkm::Id coeffLen = coeffIn.GetNumberOfValues();
      typedef typename CoeffArrayType::ValueType ValueType;
      typedef vtkm::cont::ArrayHandle< ValueType > CoeffArrayBasic;
      CoeffArrayBasic sortedArray;
      vtkm::cont::DeviceAdapterAlgorithm< DeviceTag >::Copy( coeffIn, sortedArray );
      WaveletBase::DeviceSort( sortedArray, DeviceTag() );
      
      vtkm::Id n = coeffLen - 
                   static_cast<vtkm::Id>( static_cast<vtkm::Float64>(coeffLen)/ratio );
      vtkm::Float64 nthVal = static_cast<vtkm::Float64>
                               (sortedArray.GetPortalConstControl().Get(n));
      if( nthVal < 0.0 )
        nthVal *= -1.0;
      typedef vtkm::worklet::wavelets::ThresholdWorklet ThresholdType;
      ThresholdType tw( nthVal );
      vtkm::worklet::DispatcherMapField< ThresholdType, DeviceTag > dispatcher( tw );
      dispatcher.Invoke( coeffIn );
    } 

    return 0;
  }


  // Report statistics on reconstructed array
  template< typename ArrayType, typename DeviceTag >
  vtkm::Id EvaluateReconstruction( const ArrayType &original,
                                   const ArrayType &reconstruct,
                                         DeviceTag )
  {
    #define VAL        vtkm::Float64
    #define MAKEVAL(a) (static_cast<VAL>(a))
    VAL VarOrig = WaveletBase::DeviceCalculateVariance( original, DeviceTag() );

    typedef typename ArrayType::ValueType ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType > ArrayBasic;
    ArrayBasic errorArray, errorSquare;

    // Use a worklet to calculate point-wise error, and its square
    typedef vtkm::worklet::wavelets::Differencer DifferencerWorklet;
    DifferencerWorklet dw;
    vtkm::worklet::DispatcherMapField< DifferencerWorklet > dwDispatcher( dw  );
    dwDispatcher.Invoke( original, reconstruct, errorArray );

    typedef vtkm::worklet::wavelets::SquareWorklet SquareWorklet;
    SquareWorklet sw;
    vtkm::worklet::DispatcherMapField< SquareWorklet > swDispatcher( sw );
    swDispatcher.Invoke( errorArray, errorSquare );

    VAL varErr   = WaveletBase::DeviceCalculateVariance( errorArray, DeviceTag() );
    VAL snr, decibels;
    if( varErr != 0.0 )
    {
        snr      = VarOrig / varErr;
        decibels = 10 * vtkm::Log10( snr );
    }
    else
    {
        snr      = vtkm::Infinity64();
        decibels = vtkm::Infinity64();
    }

    VAL origMax  = WaveletBase::DeviceMax( original, DeviceTag() );
    VAL origMin  = WaveletBase::DeviceMin( original, DeviceTag() );
    VAL errorMax = WaveletBase::DeviceMaxAbs( errorArray, DeviceTag() );
    VAL range    = origMax - origMin;

    VAL squareSum = WaveletBase::DeviceSum( errorSquare, DeviceTag() );
    VAL rmse      = vtkm::Sqrt( MAKEVAL(squareSum) / MAKEVAL(errorArray.GetNumberOfValues()) );

    std::cout << "Data range             = " << range << std::endl;
    std::cout << "SNR                    = " << snr << std::endl;
    std::cout << "SNR in decibels        = " << decibels << std::endl;
    std::cout << "L-infy norm            = " << errorMax 
              << ", after normalization  = " << errorMax / range << std::endl;
    std::cout << "RMSE                   = " << rmse 
              << ", after normalization  = " << rmse / range << std::endl;

    #undef MAKEVAL
    #undef VAL

    return 0;
  }

                      
  // Compute the book keeping array L for 1D wavelet decomposition
  void ComputeL( vtkm::Id               sigInLen, 
                 vtkm::Id               nLev, 
                 std::vector<vtkm::Id>  &L )
  {
    size_t nLevels = static_cast<size_t>( nLev );   // cast once
    L.resize( nLevels + 2 );
    L[ nLevels+1 ] = sigInLen;
    L[ nLevels   ] = sigInLen;
    for( size_t i = nLevels; i > 0; i-- )
    {
      L[i-1] = WaveletBase::GetApproxLength( L[i] );
      L[i]   = WaveletBase::GetDetailLength( L[i] );
    }
  }
  // Compute the book keeping array L for 2D wavelet decomposition
  void ComputeL2( vtkm::Id               inX,
                  vtkm::Id               inY,
                  vtkm::Id               nLev, 
                  std::vector<vtkm::Id>  &L )
  {
    size_t nLevels = static_cast<size_t>( nLev );    
    L.resize( nLevels*6 + 4 );
    L[        nLevels*6 + 0 ] = inX;
    L[        nLevels*6 + 1 ] = inY;
    L[        nLevels*6 + 2 ] = inX;
    L[        nLevels*6 + 3 ] = inY;

    for( size_t i = nLevels; i > 0; i-- )
    {
      // cA
      L[ i*6 - 6 ] = WaveletBase::GetApproxLength( L[ i*6 + 0 ]);
      L[ i*6 - 5 ] = WaveletBase::GetApproxLength( L[ i*6 + 1 ]);

      // cDh
      L[ i*6 - 4 ] = WaveletBase::GetApproxLength( L[ i*6 + 0 ]);
      L[ i*6 - 3 ] = WaveletBase::GetDetailLength( L[ i*6 + 1 ]);

      // cDv
      L[ i*6 - 2 ] = WaveletBase::GetDetailLength( L[ i*6 + 0 ]);
      L[ i*6 - 1 ] = WaveletBase::GetApproxLength( L[ i*6 + 1 ]);

      // cDv - overwrites previous value!
      L[ i*6 - 0 ] = WaveletBase::GetDetailLength( L[ i*6 + 0 ]);
      L[ i*6 + 1 ] = WaveletBase::GetDetailLength( L[ i*6 + 1 ]);
    }
  }


  // Compute the length of coefficients for 1D transforms
  vtkm::Id ComputeCoeffLength( std::vector<vtkm::Id> &L,
                               vtkm::Id nLevels )
  {
    vtkm::Id sum = L[0];        // 1st level cA
    for( size_t i = 1; i <= size_t(nLevels); i++ )
      sum += L[i];
    return sum;
  }
  // Compute the length of coefficients for 2D transforms
  vtkm::Id ComputeCoeffLength2( std::vector<vtkm::Id> &L,
                               vtkm::Id nLevels )
  {
    vtkm::Id sum = (L[0] * L[1]);           // 1st level cA
    for( size_t i = 1; i <= size_t(nLevels); i++ )
    {
      sum += L[ i*6 - 4 ] * L[ i*6 - 3 ];   // cDh
      sum += L[ i*6 - 2 ] * L[ i*6 - 1 ];   // cDv
      sum += L[ i*6 - 0 ] * L[ i*6 + 1 ];   // cDd
    }
    return sum;
  }

  // Compute approximate coefficient length at a specific level
  vtkm::Id GetApproxLengthLevN( vtkm::Id sigInLen, vtkm::Id levN )
  {
    vtkm::Id cALen = sigInLen;
    for( vtkm::Id i = 0; i < levN; i++ )
    {
      cALen = WaveletBase::GetApproxLength( cALen );
      if( cALen == 0 )    
        return cALen;
    }

    return cALen;
  }


};    // class WaveletCompressor

}     // namespace worklet
}     // namespace vtkm

#endif 
