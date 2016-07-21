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

#ifndef vtk_m_filter_internal_waveletdwt_h
#define vtk_m_filter_internal_waveletdwt_h

//#include <vtkm/worklet/WorkletMapField.h>
//#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/filter/internal/WaveletBase.h>

#include <vtkm/worklet/WaveletTransforms.h>

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace filter {
namespace internal {

class WaveletDWT : public WaveletBase
{
public:

  // Constructor
  WaveletDWT( const std::string &w_name ) : WaveletBase( w_name ) {} 


  // Func: Extend 1D signal
  template< typename SigInArrayType, typename SigExtendedArrayType >
  vtkm::Id Extend1D( const SigInArrayType                     &sigIn,   // Input
                     SigExtendedArrayType                     &sigOut,  // Output
                     vtkm::Id                                 addLen,
                     vtkm::filter::internal::DWTMode          leftExtMethod,
                     vtkm::filter::internal::DWTMode          rightExtMethod )
  { 
    typedef typename SigInArrayType::ValueType      ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType >    ExtensionArrayType;
    ExtensionArrayType leftExtend, rightExtend;
    leftExtend.Allocate( addLen );
    rightExtend.Allocate( addLen );

    typedef typename ExtensionArrayType::PortalControl       PortalType;
    typedef typename ExtensionArrayType::PortalConstControl  PortalConstType;
    typedef typename SigInArrayType::PortalConstControl      SigInPortalConstType;

    typedef vtkm::cont::ArrayHandleConcatenate< ExtensionArrayType, SigInArrayType> 
            ArrayConcat;

    PortalType leftExtendPortal       = leftExtend.GetPortalControl();
    PortalType rightExtendPortal      = rightExtend.GetPortalControl();
    SigInPortalConstType sigInPortal  = sigIn.GetPortalConstControl();
    vtkm::Id sigInLen                 = sigIn.GetNumberOfValues();

    switch( leftExtMethod )
    {
      case vtkm::filter::internal::SYMH:
      {
          for( vtkm::Id count = 0; count < addLen; count++ )
            leftExtendPortal.Set( count, sigInPortal.Get( addLen - count - 1) );
          break;
      }
      case vtkm::filter::internal::SYMW:
      {
          for( vtkm::Id count = 0; count < addLen; count++ )
            leftExtendPortal.Set( count, sigInPortal.Get( addLen - count ) );
          break;
      }
      default:
      {
        // throw out an error
        return 1;
      }
    }

    switch( rightExtMethod )
    {
      case vtkm::filter::internal::SYMH:
      {
          for( vtkm::Id count = 0; count < addLen; count++ )
            rightExtendPortal.Set( count, sigInPortal.Get( sigInLen - count - 1 ) );
          break;
      }
      case SYMW:
      {
          for( vtkm::Id count = 0; count < addLen; count++ )
            rightExtendPortal.Set( count, sigInPortal.Get( sigInLen - count - 2 ) );
          break;
      }
      default:
      {
        // throw out an error
        return 1;
      }
    }

    ArrayConcat leftOn( leftExtend, sigIn );    
    sigOut = vtkm::cont::make_ArrayHandleConcatenate< ArrayConcat, ExtensionArrayType >
                  (leftOn, rightExtend );

    return 0;
  }



  // Func:
  // Performs one level of 1D discrete wavelet transform 
  // It takes care of boundary conditions, etc.
  template< typename SignalArrayType, typename CoeffArrayType>
  vtkm::Id DWT1D( const SignalArrayType &sigIn,     // Input
                  CoeffArrayType        &coeffOut,  // Output: cA followed by cD
                  vtkm::Id              L[3] )      // Output: how many cA and cD.
  {

    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if( GetWaveletMaxLevel( sigInLen ) < 1 )
    {
      // throw an error
      std::cerr << "Cannot transform signal of length " << sigInLen << std::endl;
      return -1;
    } 

    L[0] = this->GetApproxLength( sigInLen );
    L[1] = this->GetDetailLength( sigInLen );
    L[2] = sigInLen;

    VTKM_ASSERT( L[0] + L[1] == L[2] );

    vtkm::Id filterLen = this->filter->GetFilterLength();

    bool doSymConv = false;
    if( this->filter->isSymmetric() )
    {
      if( ( this->wmode == SYMW && ( filterLen % 2 != 0 ) ) ||
          ( this->wmode == SYMH && ( filterLen % 2 == 0 ) ) )
        doSymConv = true;
    }

    vtkm::Id sigConvolvedLen = L[0] + L[1];     // approx + detail coeffs
    vtkm::Id addLen;                            // for extension
    bool oddLow  = true;
    bool oddHigh = true;
    if( filterLen % 2 != 0 )
      oddLow = false;
    if( doSymConv )
    {
      addLen = filterLen / 2;
      if( sigInLen % 2 != 0 )
        sigConvolvedLen += 1;
    }
    else
      addLen = filterLen - 1; 
  
    vtkm::Id sigExtendedLen = sigInLen + 2 * addLen;

    typedef typename SignalArrayType::ValueType                   SigInValueType;
    typedef vtkm::cont::ArrayHandle<SigInValueType>               SignalArrayTypeTmp;
    typedef vtkm::cont::ArrayHandleConcatenate< SignalArrayTypeTmp, SignalArrayTypeTmp> 
                ArrayConcat;
    typedef vtkm::cont::ArrayHandleConcatenate< ArrayConcat, SignalArrayTypeTmp > 
                ArrayConcat2;
    ArrayConcat2 sigInExtended;

    this->Extend1D( sigIn, sigInExtended, addLen, this->wmode, this->wmode ); 

    // Coefficients in coeffOutTmp are interleaving, 
    // e.g. cA are at 0, 2, 4...; cD are at 1, 3, 5...
    CoeffArrayType coeffOutTmp;

    
    // initialize a worklet
    vtkm::worklet::ForwardTransform forwardTransform;
    forwardTransform.SetFilterLength( filterLen );
    forwardTransform.SetCoeffLength( L[0], L[1] );
    forwardTransform.SetOddness( oddLow, oddHigh );

    vtkm::worklet::DispatcherMapField<vtkm::worklet::ForwardTransform> 
        dispatcher(forwardTransform);
    dispatcher.Invoke( sigInExtended, 
                       this->filter->GetLowDecomposeFilter(),
                       this->filter->GetHighDecomposeFilter(),
                       coeffOutTmp );

    // Separate cA and cD.
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id > IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, CoeffArrayType > PermutArrayType;

    IdArrayType approxIndices( 0, 2, L[0] );
    IdArrayType detailIndices( 1, 2, L[1] );
    PermutArrayType cATmp( approxIndices, coeffOutTmp );
    PermutArrayType cDTmp( detailIndices, coeffOutTmp );

    typedef vtkm::cont::ArrayHandleConcatenate< PermutArrayType, PermutArrayType >
                PermutArrayConcatenated;
    PermutArrayConcatenated coeffTmp( cATmp, cDTmp );

    vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(
        coeffTmp, coeffOut );

    return 0;  
  }
    
    
  // Func: 
  // Performs one level of inverse wavelet transform
  // It takes care of boundary conditions, etc.
  template< typename CoeffArrayType, typename SignalArrayType>
  vtkm::Id IDWT1D( const CoeffArrayType  &coeffIn,     // Input, cA followed by cD
                   vtkm::Id              L[3],         // Input, how many cA and cD
                   SignalArrayType       &sigOut )     // Output
  {
    VTKM_ASSERT( coeffIn.GetNumberOfValues() == L[2] );

    vtkm::Id filterLen = this->filter->GetFilterLength();
    bool doSymConv = false;
    vtkm::filter::internal::DWTMode cALeftMode  = this->wmode;
    vtkm::filter::internal::DWTMode cARightMode = this->wmode;
    vtkm::filter::internal::DWTMode cDLeftMode  = this->wmode;
    vtkm::filter::internal::DWTMode cDRightMode = this->wmode;
  
    if( this->filter->isSymmetric() )
    {
      if(( this->wmode == SYMW && (filterLen % 2 != 0) ) || 
         ( this->wmode == SYMH && (filterLen % 2 == 0) ) )
      {
        doSymConv = true;

        if( this->wmode == SYMH )
        {
          cDLeftMode = ASYMH;
          if( L[2] % 2 != 0 )
          {
            cARightMode = SYMW;
            cDRightMode = ASYMW;
          }
          else
            cDRightMode = ASYMH;
        }
        else
        {
          cDLeftMode = SYMH;
          if( L[2] % 2 != 0 )
          {
            cARightMode = SYMW;
            cDRightMode = SYMH;
          }
          else
            cARightMode = SYMH;
        }
      }
    } 

    vtkm::Id cATempLen, cDTempLen, reconTempLen;
    vtkm::Id addLen = 0;
    vtkm::Id cDPadLen  = 0;
    if( doSymConv )   // extend cA and cD
    {
      addLen = filterLen / 2;
      if( (L[0] > L[1]) && (this->wmode == SYMH) )
        cDPadLen = L[0];
      cATempLen = L[0] + 2 * addLen;
      cDTempLen = cATempLen;  // even length signal here
    }
    else              // not extend cA and cD
    {
      cATempLen = L[0];
      cDTempLen = L[1];
    }
    reconTempLen = L[2];
    if( reconTempLen % 2 != 0 )
      reconTempLen++;

    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >                       IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, CoeffArrayType > PermutArrayType;

    // Separate cA and cD
    IdArrayType approxIndices( 0,    1, L[0] );
    IdArrayType detailIndices( L[0], 1, L[1] );
    PermutArrayType cA( approxIndices, coeffIn );
    PermutArrayType cD( detailIndices, coeffIn );
    

    typedef typename CoeffArrayType::ValueType                    CoeffValueType;
    typedef vtkm::cont::ArrayHandle<CoeffValueType>               ExtensionArrayType;
    typedef vtkm::cont::ArrayHandleConcatenate< ExtensionArrayType, CoeffArrayType > 
                ArrayConcat;
    typedef vtkm::cont::ArrayHandleConcatenate< ArrayConcat, ExtensionArrayType > ArrayConcat2;

    ArrayConcat2 cATemp, cDTemp;

    /*this->Extend1D( sigIn, sigInExtended, addLen, this->wmode, this->wmode ); */

    if( doSymConv )   // Actually extend cA and cD
    {
      { // make a CoeffArrayType to send into Extend1D
        CoeffArrayType cABasic;
        vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
              (cA, cABasic);
        this->Extend1D( cABasic, cATemp, addLen, cALeftMode, cARightMode );
      }
      if( cDPadLen > 0 )  
      {
        // Add back the missing final cD: 0.0
        ExtensionArrayType singleValArray;
        singleValArray.Allocate(1);
        singleValArray.GetPortalControl().Set(0, 0.0);
        vtkm::cont::ArrayHandleConcatenate< PermutArrayType, ExtensionArrayType >
            cDPad( cD, singleValArray );

        // make a CoeffArrayType to send into Extend1D
        CoeffArrayType cDBasic;
        vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
          ( cDPad, cDBasic );
        this->Extend1D( cDBasic, cDTemp, addLen, cDLeftMode, cDRightMode );
      }
      else
      {
        CoeffArrayType cDBasic;
        vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
          ( cD, cDBasic );
        this->Extend1D( cDBasic, cDTemp, addLen, cDLeftMode, cDRightMode );
      }
    } // end if( doSymConv )
    else  // Make cATemp and cDTemp from cA and cD
    {
      ExtensionArrayType zeroLenArray;
      // make correct ArrayHandle for cATemp
      { 
        CoeffArrayType cABasic;
        vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
              (cA, cABasic);
        ArrayConcat leftOn( zeroLenArray, cABasic );
        cATemp = vtkm::cont::make_ArrayHandleConcatenate( leftOn, zeroLenArray );
      }
      // make correct ArrayHandle for cDTemp
      {
        CoeffArrayType cDBasic;
        vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
              (cD, cDBasic);
        ArrayConcat leftOn( zeroLenArray, cDBasic );
        cDTemp = vtkm::cont::make_ArrayHandleConcatenate( leftOn, zeroLenArray );
      }
    }

    if( filterLen % 2 != 0 )
    {
      // Concatenate cATemp and cDTemp
      vtkm::cont::ArrayHandleConcatenate< ArrayConcat2, ArrayConcat2>
          coeffInExtended( cATemp, cDTemp );

      std::cerr << "cATemp has length: " << cATemp.GetNumberOfValues() << std::endl;
      for( vtkm::Id i = 0; i < cATemp.GetNumberOfValues(); i++ )
          std::cout << cATemp.GetPortalConstControl().Get(i) << std::endl;
      std::cerr << "cDTemp has length: " << cDTemp.GetNumberOfValues() << std::endl;
      for( vtkm::Id i = 0; i < cATemp.GetNumberOfValues(); i++ )
          std::cout << cATemp.GetPortalConstControl().Get(i) << std::endl;
      std::cerr << "coeffIn has length: " << coeffInExtended.GetNumberOfValues() << std::endl;

      // Initialize a worklet
      vtkm::worklet::InverseTransformOdd inverseXformOdd;
      inverseXformOdd.SetFilterLength( filterLen );
      inverseXformOdd.SetCALength( L[0] );
      vtkm::worklet::DispatcherMapField< vtkm::worklet::InverseTransformOdd >
          dispatcher( inverseXformOdd );
      dispatcher.Invoke( coeffInExtended,
                         this->filter->GetLowReconstructFilter(),
                         this->filter->GetHighReconstructFilter(),
                         sigOut );

       // need to take out the first L[2] values to put into sigOut
    }
    else
    {
      // need to implement the even filter length worklet first
    }

    return 0;

  }   // Finish function IDWT1D
  

};    // Finish class WaveletDWT

}     // Finish namespace internal
}     // Finish namespace filter
}     // Finish namespace vtkm

#endif 
