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
vtkm::Id 
WaveletCompressor::WaveDecompose( const SignalArrayType   &sigIn,   // Input
                                  vtkm::Id                nLevels,  // n levels of DWT
                                  CoeffArrayType          &C )
                                  //vtkm::Id                CLength,
                                  //vtkm::Id*               L )       // bookkeeping array;
                                                                    // len(L) = nLevels+2
{
  vtkm::Id sigInLen = sigIn.GetNumberOfValues();
  if( nLevels < 0 || nLevels > WaveletBase::GetWaveletMaxLevel( sigInLen ) )
  {
    std::cerr << "nLevel is not supported: " << nLevels << std::endl;
    // throw an error
  }
  if( nLevels == 0 )  // 0 levels means no transform
  {
    vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        (sigIn, C );
    return 0;
  }

  vtkm::Id* L = new vtkm::Id[ nLevels + 2 ];  // bookkeeping array;
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

  delete[] L;

  return 0;
}

vtkm::Id 
WaveletCompressor::ComputeCoeffLength( const vtkm::Id* L, vtkm::Id nLevels )
{
  vtkm::Id sum = L[0];  // 1st level cA
  for( vtkm::Id i = 1; i <= nLevels; i++ )
    sum += L[i];
  return sum;
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
                      
/*
vtkm::Id 
WaveletCompressor::WaveDecomposeSetup( vtkm::Id sigInLen, vtkm::Id nLevels,     // Input
                                       vtkm::Id* CLength, vtkm::Id* L )
{

  return 0;
}
*/


}     // Finish namespace filter
}     // Finish namespace vtkm

