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

#ifndef vtk_m_worklet_wavelets_waveletdwt_h
#define vtk_m_worklet_wavelets_waveletdwt_h

#include <vtkm/worklet/wavelets/WaveletBase.h>

#include <vtkm/worklet/wavelets/WaveletTransforms.h>

#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {
namespace wavelets {

class WaveletDWT : public WaveletBase
{
public:

  // Constructor
  WaveletDWT( const std::string &w_name ) : WaveletBase( w_name ) {} 


  // Func: Extend 1D signal
  template< typename SigInArrayType, typename SigExtendedArrayType >
  VTKM_CONT_EXPORT
  vtkm::Id Extend1D( const SigInArrayType                     &sigIn,   // Input
                     SigExtendedArrayType                     &sigOut,  // Output
                     vtkm::Id                                 addLen,
                     vtkm::worklet::wavelets::DWTMode         leftExtMethod,
                     vtkm::worklet::wavelets::DWTMode         rightExtMethod, 
                     bool                                     attachRightZero )
  { 
    typedef typename SigInArrayType::ValueType      ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType >    ExtensionArrayType;
    ExtensionArrayType                              leftExtend;
    leftExtend.Allocate( addLen );

    vtkm::Id sigInLen = sigIn.GetNumberOfValues();

    typedef vtkm::worklet::wavelets::LeftSYMHExtentionWorklet  LeftSYMH;
    typedef vtkm::worklet::wavelets::LeftSYMWExtentionWorklet  LeftSYMW;
    typedef vtkm::worklet::wavelets::RightSYMHExtentionWorklet RightSYMH;
    typedef vtkm::worklet::wavelets::RightSYMWExtentionWorklet RightSYMW;

    switch( leftExtMethod )
    {
      case vtkm::worklet::wavelets::SYMH:
      {
          LeftSYMH worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftSYMH > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case vtkm::worklet::wavelets::SYMW:
      {
          LeftSYMW worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftSYMW > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      default:
      {
        vtkm::cont::ErrorControlInternal("Left extension mode not supported!");
        return 1;
      }
    }

    ExtensionArrayType rightExtend;
    if( attachRightZero )
      rightExtend.Allocate( addLen + 1 );
    else
      rightExtend.Allocate( addLen );

    switch( rightExtMethod )
    {
      case vtkm::worklet::wavelets::SYMH:
      {
          RightSYMH worklet( sigInLen );
          vtkm::worklet::DispatcherMapField< RightSYMH > dispatcher( worklet );
          dispatcher.Invoke( rightExtend, sigIn );
          break;
      }
      case SYMW:
      {
          RightSYMW worklet( sigInLen );
          vtkm::worklet::DispatcherMapField< RightSYMW > dispatcher( worklet );
          dispatcher.Invoke( rightExtend, sigIn );
          break;
      }
      default:
      {
        vtkm::cont::ErrorControlInternal("Right extension mode not supported!");
        return 1;
      }
    }

    if( attachRightZero )
    {
      typedef vtkm::worklet::wavelets::AssignZeroWorklet ZeroWorklet;
      ZeroWorklet worklet( addLen );
      vtkm::worklet::DispatcherMapField< ZeroWorklet > dispatcher( worklet );
      dispatcher.Invoke( rightExtend );
    }

    typedef vtkm::cont::ArrayHandleConcatenate< ExtensionArrayType, SigInArrayType> 
            ArrayConcat;
    ArrayConcat leftOn( leftExtend, sigIn );    
    sigOut = vtkm::cont::make_ArrayHandleConcatenate( leftOn, rightExtend );

    return 0;
  }


  // Func:
  // Performs one level of 1D discrete wavelet transform 
  // It takes care of boundary conditions, etc.
  template< typename SignalArrayType, typename CoeffArrayType>
  VTKM_CONT_EXPORT
  vtkm::Id DWT1D( const SignalArrayType &sigIn,     // Input
                  CoeffArrayType        &coeffOut,  // Output: cA followed by cD
                  std::vector<vtkm::Id> &L )        // Output: how many cA and cD.
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    if( GetWaveletMaxLevel( sigInLen ) < 1 )
    {
      vtkm::cont::ErrorControlInternal( "Signal is too short to perform DWT!" ); 
      return -1;
    } 

    VTKM_ASSERT( L.size() == 3 );
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

    typedef typename SignalArrayType::ValueType             SigInValueType;
    typedef vtkm::cont::ArrayHandle<SigInValueType>         SigInBasic;
    //SigInBasic sigInExtended;

    typedef vtkm::cont::ArrayHandleConcatenate< SigInBasic, SignalArrayType >
            ConcatType1;
    typedef vtkm::cont::ArrayHandleConcatenate< ConcatType1, SigInBasic >
            ConcatType2;

    ConcatType2 sigInExtended;

    this->Extend1D( sigIn, sigInExtended, addLen, 
                    WaveletBase::wmode, WaveletBase::wmode, false ); 
    VTKM_ASSERT( sigInExtended.GetNumberOfValues() == sigExtendedLen );

    // initialize a worklet
    vtkm::worklet::wavelets::ForwardTransform forwardTransform;
    forwardTransform.SetFilterLength( filterLen );
    forwardTransform.SetCoeffLength( L[0], L[1] );
    forwardTransform.SetOddness( oddLow, oddHigh );

    coeffOut.Allocate( sigInExtended.GetNumberOfValues() );
    vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::ForwardTransform> 
        dispatcher(forwardTransform);
    dispatcher.Invoke( sigInExtended, 
                       WaveletBase::filter->GetLowDecomposeFilter(),
                       WaveletBase::filter->GetHighDecomposeFilter(),
                       coeffOut );

    VTKM_ASSERT( L[0] + L[1] <= coeffOut.GetNumberOfValues() );
    coeffOut.Shrink( L[0] + L[1] );
    
    return 0;  
  } // Function DWT1D
    

  // Func: 
  // Performs one level of inverse wavelet transform
  // It takes care of boundary conditions, etc.
  template< typename CoeffArrayType, typename SignalArrayType>
  VTKM_CONT_EXPORT
  vtkm::Id IDWT1D( const CoeffArrayType  &coeffIn,     // Input, cA followed by cD
                   std::vector<vtkm::Id> &L,           // Input, how many cA and cD
                   SignalArrayType       &sigOut )     // Output
  {
    VTKM_ASSERT( L.size() == 3 );
    VTKM_ASSERT( coeffIn.GetNumberOfValues() == L[2] );

    vtkm::Id filterLen = WaveletBase::filter->GetFilterLength();
    bool doSymConv = false;
    vtkm::worklet::wavelets::DWTMode cALeftMode  = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cARightMode = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDLeftMode  = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDRightMode = WaveletBase::wmode;
  
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
        cDPadLen = L[0];  // SYMH is rare
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

    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      IdArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< IdArrayType, CoeffArrayType > 
            PermutArrayType;

    // Separate cA and cD
    IdArrayType approxIndices( 0,    1, L[0] );
    IdArrayType detailIndices( L[0], 1, L[1] );
    PermutArrayType cA( approxIndices, coeffIn );
    PermutArrayType cD( detailIndices, coeffIn );
    

    typedef typename CoeffArrayType::ValueType                    CoeffValueType;
    typedef vtkm::cont::ArrayHandle< CoeffValueType >             ExtensionArrayType;
    typedef vtkm::cont::ArrayHandleConcatenate< ExtensionArrayType, PermutArrayType >
            Concat1;
    typedef vtkm::cont::ArrayHandleConcatenate< Concat1, ExtensionArrayType >
            Concat2;

    Concat2 cATemp, cDTemp;

    if( doSymConv )   // Actually extend cA and cD
    {
      this->Extend1D( cA, cATemp, addLen, cALeftMode, cARightMode, false );
      if( cDPadLen > 0 )  
      {
        /* Add back the missing final cD: 0.0
         * TODO when SYMH is needed.
         *
        ExtensionArrayType singleValArray;
        singleValArray.Allocate(1);
        singleValArray.GetPortalControl().Set(0, 0.0);
        vtkm::cont::ArrayHandleConcatenate< PermutArrayType, ExtensionArrayType >
            cDPad( cD, singleValArray );
        this->Extend1D( cDPad, cDTemp, addLen, cDLeftMode, cDRightMode );
         */
      }
      else
      {
        vtkm::Id cDTempLenWouldBe = cD.GetNumberOfValues() + 2 * addLen;
        if( cDTempLenWouldBe ==  cDTempLen )
          this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, false);
        else if( cDTempLenWouldBe ==  cDTempLen - 1 )
          this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, true);
        else
        {
          vtkm::cont::ErrorControlInternal("cDTemp Length not match!");
          return 1;
        }
      }
    }     
    else  
    { /* TODO when SYMH is needed
      WaveletBase::DeviceCopy( cA, cATemp );
      WaveletBase::DeviceCopy( cD, cDTemp );
       */
    }

    if( filterLen % 2 != 0 )
    {
      vtkm::cont::ArrayHandleConcatenate< Concat2, Concat2>
          coeffInExtended( cATemp, cDTemp );

      // Allocate memory for sigOut
      sigOut.Allocate( coeffInExtended.GetNumberOfValues() );

      // Initialize a worklet
      vtkm::worklet::wavelets::InverseTransformOdd inverseXformOdd;
      inverseXformOdd.SetFilterLength( filterLen );
      inverseXformOdd.SetCALength( L[0], cATempLen );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformOdd>
            dispatcher( inverseXformOdd );
      dispatcher.Invoke( coeffInExtended,
                         WaveletBase::filter->GetLowReconstructFilter(),
                         WaveletBase::filter->GetHighReconstructFilter(),
                         sigOut );

      VTKM_ASSERT( sigOut.GetNumberOfValues() >= L[2] );
      sigOut.Shrink( L[2] );
    }
    else
    {
      // TODO: implement for even length filter
    }

    return 0;

  }   // function IDWT1D
  

  // Func:
  // Performs one level of 2D discrete wavelet transform 
  // It takes care of boundary conditions, etc.
  // N.B.
  //  L[0] == L[2]
  //  L[1] == L[5]
  //  L[3] == L[7]
  //  L[4] == L[6]
  //
  //      ____L[0]_______L[4]____
  //      |          |          |
  // L[1] |  cA      |  cDv     | L[5]
  //      |  (LL)    |  (HL)    |
  //      |          |          |
  //      |---------------------|
  //      |          |          |
  //      |  cDh     |  cDd     | L[7]
  // L[3] |  (LH)    |  (HH)    |
  //      |          |          |
  //      |__________|__________|
  //         L[2]       L[6]
  //
  template< typename InputArrayType, typename OutputArrayType>
  VTKM_CONT_EXPORT
  vtkm::Id DWT2D( const InputArrayType    &sigIn,     // Input array
                  vtkm::Id                inXLen,     // Input X length
                  vtkm::Id                inYLen,     // Input Y length
                  OutputArrayType         &coeffOut,  // Output coeff array
                  std::vector<vtkm::Id>   &L )        // Output coeff layout
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    VTKM_ASSERT( sigInLen == inXLen * inYLen );

    VTKM_ASSERT( L.size() == 10 );
    L[0] = WaveletBase::GetApproxLength( inXLen );    L[2] = L[0];
    L[1] = WaveletBase::GetApproxLength( inYLen );    L[5] = L[1];
    L[3] = WaveletBase::GetDetailLength( inYLen );    L[7] = L[3];
    L[4] = WaveletBase::GetDetailLength( inXLen );    L[6] = L[4];
    L[8] = inXLen;                                    L[9] = inYLen;

    typedef typename InputArrayType::ValueType             InputValueType;
    typedef vtkm::cont::ArrayHandle<InputValueType>        BasicArrayType;

    // Define a few more types
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      CountingArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< CountingArrayType, InputArrayType > 
              CountingInputPermute;
    typedef vtkm::cont::ArrayHandlePermutation< CountingArrayType, BasicArrayType > 
              CountingBasicPermute;

    std::vector<vtkm::Id> xL(3, 0);

    // Transform rows
    //    Storage for coeffs after X transform
    //    Note: DWT1D decides (L[0] + L[4] == inXLen) at line 158
    //          so safe to assume resulting coeffs have the same length
    BasicArrayType afterXBuf;
    afterXBuf.Allocate( sigInLen );
    for(vtkm::Id y = 0; y < inYLen; y++ )
    {
      // make input, output array
      CountingArrayType       inputIndices( inXLen*y, 1, inXLen );
      CountingInputPermute    row( inputIndices, sigIn );
      BasicArrayType          output;
    
      // 1D DWT on a row
      this->DWT1D( row, output, xL );
      // copy coeffs to buffer
      WaveletBase::DeviceCopyStartX( output, afterXBuf, y * inXLen );
    }

    // Transform columns
    BasicArrayType afterYBuf;
    afterYBuf.Allocate( sigInLen );
    for( vtkm::Id x = 0; x < inXLen; x++ )
    {
      // make input, output array
      CountingArrayType       inputIndices( x, inXLen, inYLen );
      CountingBasicPermute    column( inputIndices, afterXBuf );
      BasicArrayType          output;

      // 1D DWT on a row
      this->DWT1D( column, output, xL );
      // copy coeffs to buffer. afterYBuf is column major order.
      WaveletBase::DeviceCopyStartX( output, afterYBuf, inYLen*x );
    }

    // Transpose afterYBuf to output
    coeffOut.Allocate( sigInLen );
    WaveletBase::DeviceTranspose( afterYBuf, coeffOut, inYLen, inXLen );

    return 0;
  }


  // Func:
  // Performs one level of 3D discrete wavelet transform 
  // It takes care of boundary conditions, etc.
  // coeffs are stored in the order: LLL, LLH, LHL, LHH, HLL, HLH, HHL, HHH
  /* template< typename InputArrayType, typename OutputArrayType>
  VTKM_CONT_EXPORT
  vtkm::Id DWT3D( const InputArrayType    &sigIn,     // Input array
                  vtkm::Id                XLen,       // Input X length
                  vtkm::Id                YLen,       // Input Y length
                  vtkm::Id                ZLen,       // Input Z length
                  OutputArrayType         &coeffOut,  // Output coeff array
                  vtkm::Id                L[27])      // Output coeff layout
  {
    vtkm::Id sigInLen = sigIn.GetNumberOfValues();
    VTKM_ASSERT( sigInLen == XLen * YLen * ZLen );

    return 0;
  }
  */
  
};    // class WaveletDWT

}     // namespace wavelets
}     // namespace worklet
}     // namespace vtkm

#endif 
