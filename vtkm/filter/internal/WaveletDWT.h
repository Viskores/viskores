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
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Extend1D( const SigInArrayType               &sigIn,   // Input
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
    typedef vtkm::cont::ArrayHandleConcatenate< ArrayConcat, ExtensionArrayType >  
            ArrayConcat2;

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
    ArrayConcat2 rightOn(leftOn, rightExtend );
    vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy
        ( rightOn, sigOut );

    return 0;
  }



  // Func:
  // Performs one level of 1D discrete wavelet transform 
  // It takes care of boundary conditions, etc.
  template< typename SignalArrayType, typename CoeffArrayType>
  VTKM_EXEC_CONT_EXPORT
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

    L[0] = WaveletBase::GetApproxLength( sigInLen );
    L[1] = WaveletBase::GetDetailLength( sigInLen );
    L[2] = sigInLen;

    VTKM_ASSERT( L[0] + L[1] == L[2] );

    vtkm::Id filterLen = WaveletBase::filter->GetFilterLength();

    bool doSymConv = false;
    if( WaveletBase::filter->isSymmetric() )
    {
      if( ( WaveletBase::wmode == SYMW && ( filterLen % 2 != 0 ) ) ||
          ( WaveletBase::wmode == SYMH && ( filterLen % 2 == 0 ) ) )
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
    typedef vtkm::cont::ArrayHandle<SigInValueType>               SignalArrayTypeBasic;
    typedef vtkm::cont::ArrayHandleConcatenate< SignalArrayTypeBasic, SignalArrayTypeBasic> 
                ArrayConcat;
    typedef vtkm::cont::ArrayHandleConcatenate< ArrayConcat, SignalArrayTypeBasic > 
                ArrayConcat2;

    SignalArrayTypeBasic sigInExtended;

    this->Extend1D( sigIn, sigInExtended, addLen, WaveletBase::wmode, WaveletBase::wmode ); 

    // Coefficients in coeffOutTmp are interleaving, 
    // e.g. cA are at 0, 2, 4...; cD are at 1, 3, 5...
    typedef typename CoeffArrayType::ValueType CoeffOutValueType;
    typedef vtkm::cont::ArrayHandle< CoeffOutValueType > CoeffOutArrayBasic;
    CoeffOutArrayBasic coeffOutTmp;

    
    // initialize a worklet
    vtkm::worklet::ForwardTransform forwardTransform;
    forwardTransform.SetFilterLength( filterLen );
    forwardTransform.SetCoeffLength( L[0], L[1] );
    forwardTransform.SetOddness( oddLow, oddHigh );

    vtkm::worklet::DispatcherMapField<vtkm::worklet::ForwardTransform> 
        dispatcher(forwardTransform);
    dispatcher.Invoke( sigInExtended, 
                       WaveletBase::filter->GetLowDecomposeFilter(),
                       WaveletBase::filter->GetHighDecomposeFilter(),
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
    PermutArrayConcatenated coeffOutConcat( cATmp, cDTmp );

    vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(
        coeffOutConcat, coeffOut );

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

    vtkm::Id filterLen = WaveletBase::filter->GetFilterLength();
    bool doSymConv = false;
    vtkm::filter::internal::DWTMode cALeftMode  = WaveletBase::wmode;
    vtkm::filter::internal::DWTMode cARightMode = WaveletBase::wmode;
    vtkm::filter::internal::DWTMode cDLeftMode  = WaveletBase::wmode;
    vtkm::filter::internal::DWTMode cDRightMode = WaveletBase::wmode;
  
    if( WaveletBase::filter->isSymmetric() )
    {
      if(( WaveletBase::wmode == SYMW && (filterLen % 2 != 0) ) || 
         ( WaveletBase::wmode == SYMH && (filterLen % 2 == 0) ) )
      {
        doSymConv = true;

        if( WaveletBase::wmode == SYMH )
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
      addLen = filterLen / 4;
      if( (L[0] > L[1]) && (WaveletBase::wmode == SYMH) )
        cDPadLen = L[0];
      cATempLen = L[0] + 2 * addLen;
      cDTempLen = cATempLen;  // same length
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
    typedef vtkm::cont::ArrayHandle< CoeffValueType >             CoeffArrayBasic;
    typedef vtkm::cont::ArrayHandle< CoeffValueType >             ExtensionArrayType;

    CoeffArrayBasic cATemp, cDTemp;


    if( doSymConv )   // Actually extend cA and cD
    {
      this->Extend1D( cA, cATemp, addLen, cALeftMode, cARightMode );
      if( cDPadLen > 0 )  
      {
        // Add back the missing final cD: 0.0
        ExtensionArrayType singleValArray;
        singleValArray.Allocate(1);
        singleValArray.GetPortalControl().Set(0, 0.0);
        vtkm::cont::ArrayHandleConcatenate< PermutArrayType, ExtensionArrayType >
            cDPad( cD, singleValArray );

        this->Extend1D( cDPad, cDTemp, addLen, cDLeftMode, cDRightMode );
      }
      else
      {
        this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode );

        // Attached an zero if cDTemp is shorter than cDTempLen
        if( cDTemp.GetNumberOfValues() !=  cDTempLen )
        {
          VTKM_ASSERT( cDTemp.GetNumberOfValues() ==  cDTempLen - 1 ); 
          CoeffArrayBasic singleValArray;
          singleValArray.Allocate(1);
          singleValArray.GetPortalControl().Set(0, 0.0);
          vtkm::cont::ArrayHandleConcatenate< CoeffArrayBasic, CoeffArrayBasic>
              concat1( cDTemp, singleValArray );
          CoeffArrayBasic cDStorage;
          vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
              ( concat1, cDStorage );
          cDTemp = cDStorage;
        }
      }
    } // end if( doSymConv )
    else  // Make cATemp and cDTemp from cA and cD
    {
      vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
          (cA, cATemp );
      vtkm::cont::DeviceAdapterAlgorithm< VTKM_DEFAULT_DEVICE_ADAPTER_TAG >::Copy
          (cD, cDTemp );
    }

    #if 1
    std::cerr << "cATemp has length: " << cATemp.GetNumberOfValues() << std::endl;
    for( vtkm::Id i = 0; i < cATemp.GetNumberOfValues(); i++ )
        std::cout << cATemp.GetPortalConstControl().Get(i) << std::endl;
    std::cerr << "cDTemp has length: " << cDTemp.GetNumberOfValues() << std::endl;
    for( vtkm::Id i = 0; i < cDTemp.GetNumberOfValues(); i++ )
        std::cout << cDTemp.GetPortalConstControl().Get(i) << std::endl;
    #endif

    if( filterLen % 2 != 0 )
    {
      vtkm::cont::ArrayHandleConcatenate< CoeffArrayBasic, CoeffArrayBasic>
          coeffInExtended( cATemp, cDTemp );

      // Initialize a worklet
      vtkm::worklet::InverseTransformOdd inverseXformOdd;
      inverseXformOdd.SetFilterLength( filterLen );
      inverseXformOdd.SetCALength( L[0], cATempLen );
      vtkm::worklet::DispatcherMapField< vtkm::worklet::InverseTransformOdd >
          dispatcher( inverseXformOdd );
      dispatcher.Invoke( coeffInExtended,
                         WaveletBase::filter->GetLowReconstructFilter(),
                         WaveletBase::filter->GetHighReconstructFilter(),
                         sigOut );

      sigOut.Shrink( L[2] );
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
