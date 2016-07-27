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

#ifndef vtk_m_filter_waveletcompressor_h
#define vtk_m_filter_waveletcompressor_h

#include <vtkm/filter/internal/WaveletDWT.h>

//#include <vtkm/worklet/WaveletTransforms.h>

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace filter {

template< typename DeviceAdapter >
class WaveletCompressor : public internal::WaveletDWT
{
public:

  // Constructor
  WaveletCompressor( const std::string &w_name ) : WaveletDWT( w_name ) {} 

  // Multi-level 1D wavelet decomposition
  template< typename SignalArrayType, typename CoeffArrayType>
  vtkm::Id WaveDecompose( const SignalArrayType     &sigIn,   // Input
                                vtkm::Id             nLevels,  // n levels of DWT
                                CoeffArrayType      &coeffOut,
                                vtkm::Id*            L );

  // Multi-level 1D wavelet reconstruction
  template< typename CoeffArrayType, typename SignalArrayType >
  vtkm::Id WaveReconstruct( const CoeffArrayType     &coeffIn,   // Input
                                  vtkm::Id           nLevels,    // n levels of DWT
                                  vtkm::Id*          L,
                                  SignalArrayType    &sigOut );

  // In-place Threshold Coefficients
  /*
  template< typename CoeffArrayType >
  vtkm::Id SquashCoeffs( CoeffArrayType coeffs )
  {
    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;
    Algorithm::Sort( coeffs );
    
    return 0;
  }
  */
                      
  // Compute the book keeping array L for 1D wavelet decomposition
  void ComputeL( vtkm::Id sigInLen, vtkm::Id nLevels, vtkm::Id* L );

  // Compute the length of coefficients
  vtkm::Id ComputeCoeffLength( const vtkm::Id* L, vtkm::Id nLevels );

  // Compute approximate coefficient length at a specific level
  vtkm::Id GetApproxLengthLevN( vtkm::Id sigInLen, vtkm::Id levN );



};    // Finish class WaveletCompressor

}     // Finish namespace filter
}     // Finish namespace vtkm

#include <vtkm/filter/WaveletCompressor.hxx>

#endif 
