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

#include <vtkm/filter/WaveletCompressor.h>

namespace vtkm {
namespace filter {


template< typename SignalArrayType, typename CoeffArrayType>
VTKM_EXEC_CONT_EXPORT
vtkm::Id 
WaveletCompressor::WaveDecompose( const SignalArrayType   &sigIn,    // Input
                                  vtkm::Id                 nLevels,  // n levels of DWT
                                  CoeffArrayType          &C, 
                                  vtkm::Id*                L )       // bookkeeping array;
                                                                    // len(L) = nLevels+2
{
  vtkm::Id sigInLen = sigIn.GetNumberOfValues();
  if( nLevels < 1 || nLevels > WaveletBase::GetWaveletMaxLevel( sigInLen ) )
  {
    std::cerr << "nLevel is not supported: " << nLevels << std::endl;
    // throw an error
  }
  /*
  if( nLevels == 0 )  // 0 levels means no transform
  {
    vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        (sigIn, C );
    return 0;
  }
  */

  this->ComputeL( sigInLen, nLevels, L );
  vtkm::Id CLength = this->ComputeCoeffLength( L, nLevels );

  // Use 64bit floats for intermediate calculation
  #define VAL        vtkm::Float64

  vtkm::Id sigInPtr = 0;  // pseudo pointer for the beginning of input array 
  vtkm::Id len = sigIn.GetNumberOfValues();
  vtkm::Id cALen = WaveletBase::GetApproxLength( len );
  vtkm::Id cptr;          // pseudo pointer for the beginning of output array
  vtkm::Id tlen = 0;
  vtkm::Id L1d[3];

  // Use an intermediate array
  typedef vtkm::cont::ArrayHandle< VAL > InterArrayType;
  typedef typename InterArrayType::PortalControl InterPortalType;
  InterArrayType interArray;
  interArray.Allocate( CLength );
  vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        (sigIn, interArray );

  // Define a few more types
  typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >                       IdArrayType;
  typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, InterArrayType > PermutArrayType;

  for( vtkm::Id i = nLevels; i > 0; i-- )
  {
    tlen += L[i];
    cptr = 0 + CLength - tlen - cALen;
    
    // make input array (permutation array)
    IdArrayType inputIndices( sigInPtr, 1, len );
    PermutArrayType input( inputIndices, interArray ); 
    // make output array 
    InterArrayType output;

    WaveletDWT::DWT1D( input, output, L1d );

    // update interArray
    vtkm::cont::ArrayPortalToIterators< InterPortalType > 
        outputIter( output.GetPortalControl() );
    vtkm::cont::ArrayPortalToIterators< InterPortalType > 
        interArrayIter( interArray.GetPortalControl() );
    std::copy( outputIter.GetBegin(), outputIter.GetEnd(), interArrayIter.GetBegin() + cptr );

    // update pseudo pointers
    len = cALen;
    cALen = WaveletBase::GetApproxLength( cALen );
    sigInPtr = cptr;
  }

  vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        (interArray, C );

  #undef VAL

  return 0;
}



template< typename CoeffArrayType, typename SignalArrayType >
VTKM_EXEC_CONT_EXPORT
vtkm::Id 
WaveletCompressor::WaveReconstruct( const CoeffArrayType     &coeffIn,   // Input
                                    vtkm::Id                 nLevels,    // n levels of DWT
                                    vtkm::Id*                L,
                                    SignalArrayType          &sigOut )
{
  VTKM_ASSERT( nLevels > 0 );

  vtkm::Id LLength = nLevels + 2;

  vtkm::Id L1d[3] = {L[0], L[1], 0};

  // Use 64bit floats for intermediate calculation
  #define VAL        vtkm::Float64

  // Use intermediate arrays
  typedef vtkm::cont::ArrayHandle< VAL >                                    InterArrayType;
  typedef typename InterArrayType::PortalControl InterPortalType;
  typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >                       IdArrayType;
  typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, InterArrayType > PermutArrayType;

  InterArrayType interArray;
  vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        (coeffIn, interArray );

  for( vtkm::Id i = 1; i <= nLevels; i++ )
  {
    L1d[2] = this->GetApproxLengthLevN( L[ LLength-1 ], nLevels-i );

    // Make an input array
    IdArrayType inputIndices( 0, 1, L1d[2] );
    PermutArrayType input( inputIndices, interArray ); 
    
    // Make an output array
    InterArrayType output;
    
    WaveletDWT::IDWT1D( input, L1d, output );

    // Move output to intermediate array
    vtkm::cont::ArrayPortalToIterators< InterPortalType > 
        outputIter( output.GetPortalControl() );
    vtkm::cont::ArrayPortalToIterators< InterPortalType > 
        interArrayIter( interArray.GetPortalControl() );
    std::copy( outputIter.GetBegin(), outputIter.GetEnd(), interArrayIter.GetBegin() );

    L1d[0] = L1d[2];
    L1d[1] = L[i+1];
  }

  vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        ( interArray, sigOut );
  
  #undef VAL


  return 0;
}

/*
template< typename CoeffArrayType >
VTKM_EXEC_CONT_EXPORT
vtkm::Id 
WaveletCompressor::SquashCoefficients( CoeffArrayType, &coeffIn,
                                       vtkm::Id         ratio )
{
  typedef typename CoeffArrayType::ValueType ValueType;
  vtkm::cont::ArrayHandle< ValueType > sortArray;
  vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        ( coeffIn, sortArray );
  vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Sort( sortArray );
}
*/

vtkm::Id 
WaveletCompressor::ComputeCoeffLength( const vtkm::Id* L, vtkm::Id nLevels )
{
  vtkm::Id sum = L[0];  // 1st level cA
  for( vtkm::Id i = 1; i <= nLevels; i++ )
    sum += L[i];
  return sum;
}
  
vtkm::Id 
WaveletCompressor::GetApproxLengthLevN( vtkm::Id sigInLen, vtkm::Id levN )
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
  
void 
WaveletCompressor::ComputeL( vtkm::Id sigInLen, vtkm::Id nLevels, vtkm::Id* L )
{
  L[nLevels+1] = sigInLen;
  L[nLevels]   = sigInLen;
  for( vtkm::Id i = nLevels; i > 0; i-- )
  {
    L[i-1] = WaveletBase::GetApproxLength( L[i] );
    L[i]   = WaveletBase::GetDetailLength( L[i] );
  }
}
                      

}     // Finish namespace filter
}     // Finish namespace vtkm

