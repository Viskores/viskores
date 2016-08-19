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
#include <vtkm/cont/Timer.h>

namespace vtkm {
namespace worklet {
namespace wavelets {

class WaveletDWT : public WaveletBase
{
public:

  // Constructor
  WaveletDWT( WaveletName name ) : WaveletBase( name ) {} 


  // Func: Extend 1D signal
  template< typename SigInArrayType, typename SigExtendedArrayType >
  VTKM_CONT_EXPORT
  vtkm::Id Extend1D( const SigInArrayType                     &sigIn,   // Input
                     SigExtendedArrayType                     &sigOut,  // Output
                     vtkm::Id                                 addLen,
                     vtkm::worklet::wavelets::DWTMode         leftExtMethod,
                     vtkm::worklet::wavelets::DWTMode         rightExtMethod, 
                     bool                                     attachZeroRightLeft, 
                     bool                                     attachZeroRightRight )
  { 
    // "right extension" can be attached a zero on either end, but not both ends.
    VTKM_ASSERT( !attachZeroRightRight || !attachZeroRightLeft );

    typedef typename SigInArrayType::ValueType      ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType >    ExtensionArrayType;

    ExtensionArrayType                              leftExtend;
    leftExtend.Allocate( addLen );

    vtkm::Id sigInLen = sigIn.GetNumberOfValues();

    typedef vtkm::worklet::wavelets::LeftSYMHExtentionWorklet  LeftSYMH;
    typedef vtkm::worklet::wavelets::LeftSYMWExtentionWorklet  LeftSYMW;
    typedef vtkm::worklet::wavelets::RightSYMHExtentionWorklet RightSYMH;
    typedef vtkm::worklet::wavelets::RightSYMWExtentionWorklet RightSYMW;
    typedef vtkm::worklet::wavelets::LeftASYMHExtentionWorklet  LeftASYMH;
    typedef vtkm::worklet::wavelets::LeftASYMWExtentionWorklet  LeftASYMW;
    typedef vtkm::worklet::wavelets::RightASYMHExtentionWorklet RightASYMH;
    typedef vtkm::worklet::wavelets::RightASYMWExtentionWorklet RightASYMW;

    switch( leftExtMethod )
    {
      case SYMH:
      {
          LeftSYMH worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftSYMH > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case SYMW:
      {
          LeftSYMW worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftSYMW > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case ASYMH:
      {
          LeftASYMH worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftASYMH > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case ASYMW:
      {
          LeftASYMW worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftASYMW > dispatcher( worklet );
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

    if( !attachZeroRightLeft ) // no attach zero, or only attach on RightRight
    {
      // Allocate memory
      if( attachZeroRightRight )
        rightExtend.Allocate( addLen + 1 );
      else                  
        rightExtend.Allocate( addLen );

      switch( rightExtMethod )
      {
        case SYMH:
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
        case ASYMH:
        {
            RightASYMH worklet( sigInLen );
            vtkm::worklet::DispatcherMapField< RightASYMH > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigIn );
            break;
        }
        case ASYMW:
        {
            RightASYMW worklet( sigInLen );
            vtkm::worklet::DispatcherMapField< RightASYMW > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigIn );
            break;
        }
        default:
        {
          vtkm::cont::ErrorControlInternal("Right extension mode not supported!");
          return 1;
        }
      }
      if( attachZeroRightRight )
        WaveletBase::DeviceAssignZero( rightExtend, addLen );
    }
    else    // attachZeroRightLeft mode
    {
      typedef vtkm::cont::ArrayHandleConcatenate<SigInArrayType, ExtensionArrayType>
                                                      ConcatArray;
      // attach a zero at the end of sigIn
      ExtensionArrayType      singleValArray;
      singleValArray.Allocate(1);
      WaveletBase::DeviceAssignZero( singleValArray, 0 );
      ConcatArray             sigInPlusOne( sigIn, singleValArray );

      // allocate memory for extension
      rightExtend.Allocate( addLen );

      switch( rightExtMethod )
      {
        case SYMH:
        {
            RightSYMH worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightSYMH > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        case SYMW:
        {
            RightSYMW worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightSYMW > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        case ASYMH:
        {
            RightASYMH worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightASYMH > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        case ASYMW:
        {
            RightASYMW worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightASYMW > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        default:
        {
          vtkm::cont::ErrorControlInternal("Right extension mode not supported!");
          return 1;
        }
      }

      // make a copy of rightExtend with a zero attached to the left
      ExtensionArrayType rightExtendPlusOne;
      rightExtendPlusOne.Allocate( addLen + 1 );
      WaveletBase::DeviceCopyStartX( rightExtend, rightExtendPlusOne, 1 );
      WaveletBase::DeviceAssignZero( rightExtendPlusOne, 0 );
      rightExtend = rightExtendPlusOne ;
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

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();

    bool doSymConv = false;
    if( WaveletBase::filter.isSymmetric() )
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
                    WaveletBase::wmode, WaveletBase::wmode, false, false ); 
    VTKM_ASSERT( sigInExtended.GetNumberOfValues() == sigExtendedLen );

    // initialize a worklet for forward transform
    vtkm::worklet::wavelets::ForwardTransform forwardTransform;
    forwardTransform.SetFilterLength( filterLen );
    forwardTransform.SetCoeffLength( L[0], L[1] );
    forwardTransform.SetOddness( oddLow, oddHigh );

    coeffOut.Allocate( sigInExtended.GetNumberOfValues() );
    vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::ForwardTransform> 
        dispatcher(forwardTransform);
    dispatcher.Invoke( sigInExtended, 
                       WaveletBase::filter.GetLowDecomposeFilter(),
                       WaveletBase::filter.GetHighDecomposeFilter(),
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
    VTKM_ASSERT( L[2] == coeffIn.GetNumberOfValues() );

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool doSymConv = false;
    vtkm::worklet::wavelets::DWTMode cALeftMode  = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cARightMode = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDLeftMode  = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDRightMode = WaveletBase::wmode;
  
    if( WaveletBase::filter.isSymmetric() )
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

    vtkm::Id cATempLen, cDTempLen;  //, reconTempLen;
    vtkm::Id addLen = 0;
    vtkm::Id cDPadLen  = 0;
    if( doSymConv )   // extend cA and cD
    {
      addLen = filterLen / 4;   // addLen == 0 for Haar kernel
      if( (L[0] > L[1]) && (WaveletBase::wmode == SYMH) )
        cDPadLen = L[0];  

      cATempLen = L[0] + 2 * addLen;
      cDTempLen = cATempLen;  // same length
    }
    else              // not extend cA and cD
    {                 //  (biorthogonal kernels won't come into this case)
      cATempLen = L[0];
      cDTempLen = L[1];
    }

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
      // first extend cA to be cATemp
      this->Extend1D( cA, cATemp, addLen, cALeftMode, cARightMode, false, false );

      // Then extend cD based on extension needs
      if( cDPadLen > 0 )  
      {
        // Add back the missing final cD, 0.0, before doing extension
        this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, true, false );
      }
      else
      {
        vtkm::Id cDTempLenWouldBe = L[1] + 2 * addLen;
        if( cDTempLenWouldBe ==  cDTempLen )
        { 
          this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, false);
        }
        else if( cDTempLenWouldBe ==  cDTempLen - 1 )
        { 
          this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, true );
        }
        else
        {
          vtkm::cont::ErrorControlInternal("cDTemp Length not match!");
          return 1;
        }
      }
    }     
    else    // !doSymConv (biorthogonals kernel won't come into this case)
    { 
      // make cATemp
      ExtensionArrayType dummyArray;
      dummyArray.Allocate(0);
      Concat1 cALeftOn( dummyArray, cA );
      cATemp = vtkm::cont::make_ArrayHandleConcatenate< Concat1, ExtensionArrayType >
               ( cALeftOn, dummyArray );
      
      // make cDTemp
      Concat1 cDLeftOn( dummyArray, cD );
      cDTemp = vtkm::cont::make_ArrayHandleConcatenate< Concat1, ExtensionArrayType >
               ( cDLeftOn, dummyArray );
    }

    // make sure signal extension went as expected
    VTKM_ASSERT( cATemp.GetNumberOfValues() == cATempLen );
    VTKM_ASSERT( cDTemp.GetNumberOfValues() == cDTempLen );

    vtkm::cont::ArrayHandleConcatenate< Concat2, Concat2>
        coeffInExtended( cATemp, cDTemp );
    // Allocate memory for sigOut
    sigOut.Allocate( cATempLen + cDTempLen );

    if( filterLen % 2 != 0 )
    {
      vtkm::worklet::wavelets::InverseTransformOdd inverseXformOdd;
      inverseXformOdd.SetFilterLength( filterLen );
      inverseXformOdd.SetCALength( L[0], cATempLen );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformOdd>
            dispatcher( inverseXformOdd );
      dispatcher.Invoke( coeffInExtended,
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         sigOut );
    }
    else
    {
      vtkm::worklet::wavelets::InverseTransformEven inverseXformEven
            ( filterLen, L[0], cATempLen, !doSymConv );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformEven>
            dispatcher( inverseXformEven );
      dispatcher.Invoke( coeffInExtended,
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         sigOut );
    }

    sigOut.Shrink( L[2] );

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

    // Define a few types
    typedef typename InputArrayType::ValueType             InputValueType;
    typedef vtkm::cont::ArrayHandle<InputValueType>        BasicArrayType;
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
    //coeffOut = afterYBuf;

    return 0;
  }

  // Perform 1 level inverse wavelet transform
  template< typename InputArrayType, typename OutputArrayType>
  VTKM_CONT_EXPORT
  vtkm::Id IDWT2D( const InputArrayType    &sigIn,     // Input: array
                   std::vector<vtkm::Id>   &L,         // Input: coeff layout
                   OutputArrayType         &sigOut)    // Output coeff array
  {
    VTKM_ASSERT( L.size() == 10 );
    vtkm::Id inXLen = L[0] + L[4];   
    vtkm::Id inYLen = L[1] + L[3];
    vtkm::Id sigLen = inXLen * inYLen;
    VTKM_ASSERT( sigLen == sigIn.GetNumberOfValues() );

    // Define a few types
    typedef typename InputArrayType::ValueType               InputValueType;
    typedef vtkm::cont::ArrayHandle<InputValueType>          BasicArrayType;
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >      CountingArrayType;
    typedef vtkm::cont::ArrayHandlePermutation< CountingArrayType, InputArrayType > 
              CountingInputPermute;
    typedef vtkm::cont::ArrayHandlePermutation< CountingArrayType, BasicArrayType > 
              CountingBasicPermute;

    std::vector<vtkm::Id> xL(3, 0);

    // First IDWT on columns
    BasicArrayType afterYBuf;
    afterYBuf.Allocate( sigLen );    
    // make a bookkeeping array
    xL[0] = L[1];
    xL[1] = L[3];
    xL[2] = L[9];

    for( vtkm::Id x = 0; x < inXLen; x++ )
    {
      // make input, output array
      CountingArrayType       inputIdx( x, inXLen, inYLen );
      CountingInputPermute    input( inputIdx, sigIn );
      BasicArrayType          output;

      // perform inverse DWT on a logical column
      this->IDWT1D( input, xL, output);
      // copy results to a buffer
      WaveletBase::DeviceCopyStartX( output, afterYBuf, x * inYLen );
    }

    // Second IDWT on rows
    sigOut.Allocate( sigLen );
    // make a bookkeeping array
    xL[0] = L[0];
    xL[1] = L[4];
    xL[2] = L[8];
    for( vtkm::Id y = 0; y < inYLen; y++ )
    {
      // make input, output array
      CountingArrayType     inputIdx( y, inYLen, inXLen );
      CountingBasicPermute  input( inputIdx, afterYBuf );
      BasicArrayType        output;

      // perform inverse DWT on a logical row
      this->IDWT1D( input, xL, output);
      // copy results to a buffer
      WaveletBase::DeviceCopyStartX( output, sigOut, y * inXLen );
    }

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
