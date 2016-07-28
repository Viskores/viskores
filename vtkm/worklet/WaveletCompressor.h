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
  WaveletCompressor( const std::string &w_name ) : WaveletDWT( w_name ) {} 

  // Multi-level 1D wavelet decomposition
  template< typename SignalArrayType, typename CoeffArrayType>
  VTKM_CONT_EXPORT
  vtkm::Id WaveDecompose( const SignalArrayType     &sigIn,   // Input
                                vtkm::Id             nLevels,  // n levels of DWT
                                CoeffArrayType      &coeffOut,
                                vtkm::Id*            L )
  {

    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if( nLevels < 1 || nLevels > WaveletBase::GetWaveletMaxLevel( sigInLen ) )
    {
      std::cerr << "nLevel is not supported: " << nLevels << std::endl;
      // TODO: throw an error
    }
    /*  0 levels means no transform
    if( nLevels == 0 )  
    {
      vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
          (sigIn, C );
      return 0;
    } */

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
          (interArray, coeffOut );

    #undef VAL

    return 0;
  }

  // Multi-level 1D wavelet reconstruction
  template< typename CoeffArrayType, typename SignalArrayType >
  VTKM_CONT_EXPORT
  vtkm::Id WaveReconstruct( const CoeffArrayType     &coeffIn,   // Input
                                  vtkm::Id           nLevels,    // n levels of DWT
                                  vtkm::Id*          L,
                                  SignalArrayType    &sigOut )
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

  // Squash coefficients smaller than a threshold
  template< typename CoeffArrayType >
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id SquashCoefficients( CoeffArrayType   &coeffIn,
                               vtkm::Id         ratio )
  {
    typedef typename CoeffArrayType::ValueType ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType > CoeffArrayBasic;
    CoeffArrayBasic sortedArray;
    WaveletBase::DeviceCopy( coeffIn, sortedArray );
    WaveletBase::DeviceSort( sortedArray );
    
    vtkm::Id n = vtkm::Ceil( static_cast<vtkm::Float64>(coeffIn.GetNumberOfValues()) / 
                             static_cast<vtkm::Float64>(ratio) );
    ValueType threshold = sortedArray.GetPortalConstControl().Get( n );
    if( threshold < 0.0 )
      threshold *= -1.0;

    sortedArray.ReleaseResources();
  
    

    return 0;
  }

                      
  // Compute the book keeping array L for 1D wavelet decomposition
  void ComputeL( vtkm::Id sigInLen, vtkm::Id nLevels, vtkm::Id* L )
  {
    L[nLevels+1] = sigInLen;
    L[nLevels]   = sigInLen;
    for( vtkm::Id i = nLevels; i > 0; i-- )
    {
      L[i-1] = WaveletBase::GetApproxLength( L[i] );
      L[i]   = WaveletBase::GetDetailLength( L[i] );
    }
  }

  // Compute the length of coefficients
  vtkm::Id ComputeCoeffLength( const vtkm::Id* L, vtkm::Id nLevels )
  {
    vtkm::Id sum = L[0];  // 1st level cA
    for( vtkm::Id i = 1; i <= nLevels; i++ )
      sum += L[i];
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
