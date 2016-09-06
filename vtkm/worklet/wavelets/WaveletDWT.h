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

  typedef vtkm::Float64 FLOAT_64;

  // Func: Extend 2D signal on left and right sides
  template< typename SigInArrayType, typename SigExtendedArrayType, typename DeviceTag >
  vtkm::Id Extend2D(const SigInArrayType                     &sigIn,   // Input
                          vtkm::Id                                 sigDimX,             
                          vtkm::Id                                 sigDimY,
                          SigExtendedArrayType                     &sigOut,  // Output
                          vtkm::Id                                 addLen,
                          vtkm::worklet::wavelets::DWTMode         leftExtMethod,
                          vtkm::worklet::wavelets::DWTMode         rightExtMethod, 
                          bool                                     attachZeroRightLeft, 
                          bool                                     attachZeroRightRight,
                          DeviceTag                                                     )
  {
    // "right extension" can be attached a zero on either end, but not both ends.
    VTKM_ASSERT( !attachZeroRightRight || !attachZeroRightLeft );

    const vtkm::Id extDimX   = addLen;
    const vtkm::Id extDimY   = sigDimY;

    vtkm::Id      outputDimX = 2 * extDimX + sigDimX; 
    if( attachZeroRightRight || attachZeroRightLeft )
      outputDimX++;   
    const vtkm::Id     outputDimY = sigDimY;

    typedef typename SigInArrayType::ValueType                 ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType >               ExtendArrayType; 

    // Work on left extension, copy result to output
    ExtendArrayType                              leftExtend;
    leftExtend.PrepareForOutput( extDimX * extDimY, DeviceTag() );
    typedef vtkm::worklet::wavelets::LeftExtentionWorklet2D  LeftWorkletType;
    LeftWorkletType leftWorklet( extDimX, extDimY, sigDimX, sigDimY, leftExtMethod );
    vtkm::worklet::DispatcherMapField< LeftWorkletType, DeviceTag > 
          dispatcher( leftWorklet );
    dispatcher.Invoke( leftExtend, sigIn );

    // Work on right extension
    typedef vtkm::worklet::wavelets::RightExtentionWorklet2D    RightWorkletType;
    ExtendArrayType rightExtend;
    if( !attachZeroRightLeft ) // no attach zero, or only attach on RightRight
    {
      vtkm::Id extDimXRight = extDimX;
      if( attachZeroRightRight )
        extDimXRight++;

      // Allocate memory
      rightExtend.PrepareForOutput( extDimXRight * extDimY, DeviceTag() );
  
      RightWorkletType rightWorklet(extDimXRight, extDimY, sigDimX, sigDimY, rightExtMethod);
      vtkm::worklet::DispatcherMapField< RightWorkletType, DeviceTag > 
            dispatcher2( rightWorklet );
      dispatcher2.Invoke( rightExtend, sigIn );

      if( attachZeroRightRight )
        WaveletBase::DeviceAssignZero2DColumn( rightExtend, extDimXRight, extDimY,
                                               extDimXRight-1, DeviceTag() );

      // Allocate memory for output
      sigOut.PrepareForOutput( outputDimX * outputDimY, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( sigIn, sigDimX, sigDimY,
                                          sigOut, outputDimX, outputDimY,
                                          extDimX, 0, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( leftExtend, extDimX, extDimY,
                                          sigOut, outputDimX, outputDimY,
                                          0, 0, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( rightExtend, extDimXRight, extDimY,
                                          sigOut, outputDimX, outputDimY,
                                          extDimX+sigDimX, 0, DeviceTag() );
    }
    else    // attachZeroRightLeft mode. 
    {
      // Make a copy of sigIn with an extra column
      ExtendArrayType     sigInWithZero;
      sigInWithZero.PrepareForOutput( (sigDimX+1) * sigDimY, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( sigIn, sigDimX, sigDimY,
                                          sigInWithZero, sigDimX+1, sigDimY,
                                          0, 0, DeviceTag() );

      // Assign zero to the column at the very right 
      WaveletBase::DeviceAssignZero2DColumn( sigInWithZero, sigDimX+1, sigDimY, 
                                             sigDimX, DeviceTag() );

      // Allocate memory for rightExtend
      rightExtend.PrepareForOutput( extDimX * extDimY, DeviceTag() );

      // Do the extension
      RightWorkletType rightWorklet( extDimX, extDimY, sigDimX+1, sigDimY, rightExtMethod );
      vtkm::worklet::DispatcherMapField< RightWorkletType, DeviceTag > 
            dispatcher5( rightWorklet );
      dispatcher5.Invoke( rightExtend, sigInWithZero );
  
      sigInWithZero.ReleaseResources();

      // Allocate memory for output
      sigOut.PrepareForOutput( outputDimX * outputDimY, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( sigIn, sigDimX, sigDimY,
                                          sigOut, outputDimX, outputDimY,
                                          extDimX, 0, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( leftExtend, extDimX, extDimY,
                                          sigOut, outputDimX, outputDimY,
                                          0, 0, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( rightExtend, extDimX, extDimY,
                                          sigOut, outputDimX, outputDimY,
                                          extDimX+sigDimX+1, 0, DeviceTag() );
    }

    return 0;
  }


  // Func: Extend 1D signal
  template< typename SigInArrayType, typename SigExtendedArrayType, typename DeviceTag >
  vtkm::Id Extend1D( const SigInArrayType                     &sigIn,   // Input
                     SigExtendedArrayType                     &sigOut,  // Output
                     vtkm::Id                                 addLen,
                     vtkm::worklet::wavelets::DWTMode         leftExtMethod,
                     vtkm::worklet::wavelets::DWTMode         rightExtMethod, 
                     bool                                     attachZeroRightLeft, 
                     bool                                     attachZeroRightRight,
                     DeviceTag                                                     )
  { 
    // "right extension" can be attached a zero on either end, but not both ends.
    VTKM_ASSERT( !attachZeroRightRight || !attachZeroRightLeft );

    typedef typename SigInArrayType::ValueType      ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType >    ExtensionArrayType;

    ExtensionArrayType                              leftExtend;
    leftExtend.PrepareForOutput( addLen, DeviceTag() );

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
          vtkm::worklet::DispatcherMapField< LeftSYMH, DeviceTag > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case SYMW:
      {
          LeftSYMW worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftSYMW, DeviceTag > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case ASYMH:
      {
          LeftASYMH worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftASYMH, DeviceTag > dispatcher( worklet );
          dispatcher.Invoke( leftExtend, sigIn );
          break;
      }
      case ASYMW:
      {
          LeftASYMW worklet( addLen );
          vtkm::worklet::DispatcherMapField< LeftASYMW, DeviceTag > dispatcher( worklet );
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
        rightExtend.PrepareForOutput( addLen+1, DeviceTag() );
      else                  
        rightExtend.PrepareForOutput( addLen, DeviceTag() );

      switch( rightExtMethod )
      {
        case SYMH:
        {
            RightSYMH worklet( sigInLen );
            vtkm::worklet::DispatcherMapField< RightSYMH, DeviceTag > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigIn );
            break;
        }
        case SYMW:
        {
            RightSYMW worklet( sigInLen );
            vtkm::worklet::DispatcherMapField< RightSYMW, DeviceTag > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigIn );
            break;
        }
        case ASYMH:
        {
            RightASYMH worklet( sigInLen );
            vtkm::worklet::DispatcherMapField< RightASYMH, DeviceTag > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigIn );
            break;
        }
        case ASYMW:
        {
            RightASYMW worklet( sigInLen );
            vtkm::worklet::DispatcherMapField< RightASYMW, DeviceTag > dispatcher( worklet );
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
        WaveletBase::DeviceAssignZero( rightExtend, addLen, DeviceTag() );
    }
    else    // attachZeroRightLeft mode
    {
      typedef vtkm::cont::ArrayHandleConcatenate<SigInArrayType, ExtensionArrayType>
                                                      ConcatArray;
      // attach a zero at the end of sigIn
      ExtensionArrayType      singleValArray;
      singleValArray.PrepareForOutput(1, DeviceTag() );
      WaveletBase::DeviceAssignZero( singleValArray, 0, DeviceTag() );
      ConcatArray             sigInPlusOne( sigIn, singleValArray );

      // allocate memory for extension
      rightExtend.PrepareForOutput( addLen, DeviceTag() );

      switch( rightExtMethod )
      {
        case SYMH:
        {
            RightSYMH worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightSYMH, DeviceTag > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        case SYMW:
        {
            RightSYMW worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightSYMW, DeviceTag > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        case ASYMH:
        {
            RightASYMH worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightASYMH, DeviceTag > dispatcher( worklet );
            dispatcher.Invoke( rightExtend, sigInPlusOne );
            break;
        }
        case ASYMW:
        {
            RightASYMW worklet( sigInLen + 1 );
            vtkm::worklet::DispatcherMapField< RightASYMW, DeviceTag > dispatcher( worklet );
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
      rightExtendPlusOne.PrepareForOutput( addLen + 1, DeviceTag() );
      WaveletBase::DeviceCopyStartX( rightExtend, rightExtendPlusOne, 1, DeviceTag() );
      WaveletBase::DeviceAssignZero( rightExtendPlusOne, 0, DeviceTag() );
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
  template< typename SignalArrayType, typename CoeffArrayType, typename DeviceTag>
  FLOAT_64 DWT1D( const SignalArrayType &sigIn,     // Input
                  CoeffArrayType        &coeffOut,  // Output: cA followed by cD
                  std::vector<vtkm::Id> &L,         // Output: how many cA and cD.
                  DeviceTag                       )
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

    typedef vtkm::cont::ArrayHandleConcatenate< SigInBasic, SignalArrayType >
            ConcatType1;
    typedef vtkm::cont::ArrayHandleConcatenate< ConcatType1, SigInBasic >
            ConcatType2;

    ConcatType2 sigInExtended;

    this->Extend1D( sigIn, sigInExtended, addLen, WaveletBase::wmode, 
                    WaveletBase::wmode, false, false, DeviceTag() ); 
    VTKM_ASSERT( sigInExtended.GetNumberOfValues() == sigExtendedLen );

    // initialize a worklet for forward transform
    vtkm::worklet::wavelets::ForwardTransform forwardTransform;
    forwardTransform.SetFilterLength( filterLen );
    forwardTransform.SetCoeffLength( L[0], L[1] );
    forwardTransform.SetOddness( oddLow, oddHigh );

    coeffOut.PrepareForOutput( sigExtendedLen, DeviceTag() );
    vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::ForwardTransform, DeviceTag> 
        dispatcher(forwardTransform);
    // put a timer
    vtkm::cont::Timer<> timer;
    dispatcher.Invoke( sigInExtended, 
                       WaveletBase::filter.GetLowDecomposeFilter(),
                       WaveletBase::filter.GetHighDecomposeFilter(),
                       coeffOut );
    vtkm::Float64 elapsedTime = timer.GetElapsedTime();  

    VTKM_ASSERT( L[0] + L[1] <= coeffOut.GetNumberOfValues() );
    coeffOut.Shrink( L[0] + L[1] );
    
    return elapsedTime;  
  } 
    

  // Func: 
  // Performs one level of inverse wavelet transform
  // It takes care of boundary conditions, etc.
  template< typename CoeffArrayType, typename SignalArrayType, typename DeviceTag >
  FLOAT_64 IDWT1D( const CoeffArrayType  &coeffIn,     // Input, cA followed by cD
                   std::vector<vtkm::Id> &L,           // Input, how many cA and cD
                   SignalArrayType       &sigOut,      // Output
bool                  print, // debug use only
                   DeviceTag                     )
  {
    VTKM_ASSERT( L.size() == 3 );
    VTKM_ASSERT( L[2] == coeffIn.GetNumberOfValues() );

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool doSymConv = false;
    vtkm::worklet::wavelets::DWTMode cALeftMode  = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cARightMode = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDLeftMode  = WaveletBase::wmode;
    vtkm::worklet::wavelets::DWTMode cDRightMode = WaveletBase::wmode;
  
    if( WaveletBase::filter.isSymmetric() ) // this is always true with the 1st 4 filters.
    {
      if(( WaveletBase::wmode == SYMW && (filterLen % 2 != 0) ) || 
         ( WaveletBase::wmode == SYMH && (filterLen % 2 == 0) ) )
      {
        doSymConv = true;   // doSymConv is always true with the 1st 4 filters.

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
      this->Extend1D( cA, cATemp, addLen, cALeftMode, cARightMode, false, false, DeviceTag() );

      // Then extend cD based on extension needs
      if( cDPadLen > 0 )  
      {
        // Add back the missing final cD, 0.0, before doing extension
        this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, true, false, DeviceTag() );
      }
      else
      {
        vtkm::Id cDTempLenWouldBe = L[1] + 2 * addLen;
        if( cDTempLenWouldBe ==  cDTempLen )
        { 
          this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, false, DeviceTag());
        }
        else if( cDTempLenWouldBe ==  cDTempLen - 1 )
        { 
          this->Extend1D( cD, cDTemp, addLen, cDLeftMode, cDRightMode, false, true , DeviceTag());
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
      dummyArray.PrepareForOutput(0, DeviceTag() );
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

if( print) 
{
  for( vtkm::Id i = 0; i < coeffInExtended.GetNumberOfValues(); i++ )
    printf( "%.2e  ",coeffInExtended.GetPortalConstControl().Get(i) );
  std::cout << std::endl;
}

    // Allocate memory for sigOut
    sigOut.PrepareForOutput( cATempLen + cDTempLen, DeviceTag() );

    vtkm::Float64 elapsedTime; 
    if( filterLen % 2 != 0 )
    {
      vtkm::worklet::wavelets::InverseTransformOdd inverseXformOdd;
      inverseXformOdd.SetFilterLength( filterLen );
      inverseXformOdd.SetCALength( L[0], cATempLen );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformOdd, DeviceTag>
            dispatcher( inverseXformOdd );
      // use a timer
      vtkm::cont::Timer<> timer;
      dispatcher.Invoke( coeffInExtended,
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         sigOut );
      elapsedTime = timer.GetElapsedTime();
    }
    else
    {
      vtkm::worklet::wavelets::InverseTransformEven inverseXformEven
            ( filterLen, L[0], cATempLen, !doSymConv );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::InverseTransformEven, DeviceTag>
            dispatcher( inverseXformEven );
      // use a timer
      vtkm::cont::Timer<> timer;
      dispatcher.Invoke( coeffInExtended,
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         sigOut );
      elapsedTime = timer.GetElapsedTime();
    }

    sigOut.Shrink( L[2] );

    return elapsedTime;
  }   
  

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
  template< typename InputArrayType, typename OutputArrayType, typename DeviceTag >
  FLOAT_64 DWT2D( const InputArrayType    &sigIn,     // Input array
                  vtkm::Id                inXLen,     // Input X length
                  vtkm::Id                inYLen,     // Input Y length
                  OutputArrayType         &coeffOut,  // Output coeff array
                  std::vector<vtkm::Id>   &L,         // Output coeff layout
                  DeviceTag                         )
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
    typedef vtkm::cont::ArrayHandleCounting< vtkm::Id >    CountingArrayType;
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
    afterXBuf.PrepareForOutput( sigInLen, DeviceTag() );

    vtkm::Float64 elapsedTime = 0.0;
    for(vtkm::Id y = 0; y < inYLen; y++ )
    {
      // make input, output array
      CountingArrayType       inputIndices( inXLen*y, 1, inXLen );
      CountingInputPermute    row( inputIndices, sigIn );
      BasicArrayType          output;
    
      // 1D DWT on a row
      elapsedTime += this->DWT1D( row, output, xL, DeviceTag() );
      // copy coeffs to buffer
      WaveletBase::DeviceCopyStartX( output, afterXBuf, y * inXLen, DeviceTag() );
    }

    // Transform columns
    BasicArrayType afterYBuf;
    afterYBuf.PrepareForOutput( sigInLen, DeviceTag() );
    for( vtkm::Id x = 0; x < inXLen; x++ )
    {
      // make input, output array
      CountingArrayType       inputIndices( x, inXLen, inYLen );
      CountingBasicPermute    column( inputIndices, afterXBuf );
      BasicArrayType          output;

      // 1D DWT on a row
      elapsedTime += this->DWT1D( column, output, xL, DeviceTag() );
      // copy coeffs to buffer. afterYBuf is column major order.
      WaveletBase::DeviceCopyStartX( output, afterYBuf, inYLen*x, DeviceTag() );
    }

    // Transpose afterYBuf to output
    coeffOut.PrepareForOutput( sigInLen, DeviceTag() );
    WaveletBase::DeviceTranspose( afterYBuf, inYLen, inXLen, 
                                  coeffOut,  inXLen, inYLen, 
                                  0, 0, DeviceTag() );

    return elapsedTime;
  }


  // Perform 1 level inverse wavelet transform
  template< typename InputArrayType, typename OutputArrayType, typename DeviceTag >
  FLOAT_64 IDWT2D( const InputArrayType    &sigIn,     // Input: array
                   std::vector<vtkm::Id>   &L,         // Input: coeff layout
                   OutputArrayType         &sigOut,    // Output coeff array
                   DeviceTag                         )
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
    afterYBuf.PrepareForOutput( sigLen, DeviceTag() );    

    // make a bookkeeping array
    xL[0] = L[1];
    xL[1] = L[3];
    xL[2] = L[9];

    vtkm::Float64 elapsedTime = 0.0;

    for( vtkm::Id x = 0; x < inXLen; x++ )
    {
      // make input, output array
      CountingArrayType       inputIdx( x, inXLen, inYLen );
      CountingInputPermute    input( inputIdx, sigIn );
      BasicArrayType          output;

      // perform inverse DWT on a logical column
      elapsedTime += this->IDWT1D( input, xL, output, false, DeviceTag() );
      // copy results to a buffer
      WaveletBase::DeviceCopyStartX( output, afterYBuf, x * inYLen, DeviceTag() );
    }

    // Second IDWT on rows
    sigOut.PrepareForOutput( sigLen, DeviceTag() );

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
      elapsedTime += this->IDWT1D( input, xL, output, false, DeviceTag() );
      // copy results to a buffer
      WaveletBase::DeviceCopyStartX( output, sigOut, y * inXLen, DeviceTag() );
    }

    return elapsedTime;
  }


  // Func:
  // Performs one level of 2D discrete wavelet transform
  // It takes care of boundary conditions.
  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  FLOAT_64 DWT2Dv2( const ArrayInType                 &sigIn,
                          vtkm::Id                    sigDimX,
                          vtkm::Id                    sigDimY,
                          ArrayOutType                &coeffOut,
                          std::vector<vtkm::Id>       &L,
                          DeviceTag                     )
  {
    VTKM_ASSERT( sigDimX * sigDimY == sigIn.GetNumberOfValues() );
    
    VTKM_ASSERT( L.size() == 10 );
    L[0] = WaveletBase::GetApproxLength( sigDimX );    L[2] = L[0];
    L[1] = WaveletBase::GetApproxLength( sigDimY );    L[5] = L[1];
    L[3] = WaveletBase::GetDetailLength( sigDimY );    L[7] = L[3];
    L[4] = WaveletBase::GetDetailLength( sigDimX );    L[6] = L[4];
    L[8] = sigDimX;                                    L[9] = sigDimY;

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    bool oddLow        = true;
    if( filterLen % 2 != 0 )
      oddLow = false;
    vtkm::Id addLen          = filterLen / 2;
    vtkm::Id sigExtendedDimX, sigExtendedDimY;

    typedef typename ArrayInType::ValueType         ValueType;
    typedef vtkm::cont::ArrayHandle<ValueType>      ArrayType;
    ArrayType   sigExtended;

    // First do transform in X direction
    sigExtendedDimX = sigDimX + 2 * addLen;
    sigExtendedDimY = sigDimY;
    this->Extend2D( sigIn, sigDimX, sigDimY, sigExtended, addLen, WaveletBase::wmode, 
                    WaveletBase::wmode, false, false, DeviceTag()  );
    vtkm::Id outDimX = sigDimX;
    vtkm::Id outDimY = sigDimY;
    ArrayType          afterX;
    afterX.PrepareForOutput( outDimX * outDimY, DeviceTag() ); 

    typedef vtkm::worklet::wavelets::ForwardTransform2D ForwardXForm;
    {
    ForwardXForm worklet( filterLen, L[0], oddLow, sigExtendedDimX, sigExtendedDimY,
                          outDimX, outDimY );
    vtkm::worklet::DispatcherMapField< ForwardXForm, DeviceTag > dispatcher( worklet );
    dispatcher.Invoke( sigExtended, 
                       WaveletBase::filter.GetLowDecomposeFilter(),
                       WaveletBase::filter.GetHighDecomposeFilter(),
                       afterX );
    }
    sigExtended.ReleaseResources();

    // Then do transform in Y direction
    ArrayType        afterXTransposed;
    afterXTransposed.PrepareForOutput( outDimX * outDimY, DeviceTag() );
    WaveletBase::DeviceTranspose( afterX, outDimX, outDimY, 
                                  afterXTransposed, outDimY, outDimX,
                                  0, 0, DeviceTag() );
    afterX.ReleaseResources();
    sigExtendedDimX = sigDimY + 2 * addLen;   // sigExtended holds transposed "afterX"
    sigExtendedDimY = sigDimX;

    this->Extend2D( afterXTransposed, outDimY, outDimX, sigExtended, addLen, 
                    WaveletBase::wmode, WaveletBase::wmode, 
                    false, false, DeviceTag() );
    afterXTransposed.ReleaseResources();

    ArrayType   afterY;
    afterY.PrepareForOutput( outDimY * outDimX, DeviceTag() );
    {
    ForwardXForm worklet2( filterLen, L[1], oddLow, sigExtendedDimX, sigExtendedDimY,
                           outDimY, outDimX );
    vtkm::worklet::DispatcherMapField< ForwardXForm, DeviceTag > dispatcher2( worklet2 );
    dispatcher2.Invoke( sigExtended, 
                        WaveletBase::filter.GetLowDecomposeFilter(),
                        WaveletBase::filter.GetHighDecomposeFilter(),
                        afterY );
    }
    sigExtended.ReleaseResources();

    // Transpose to output
    coeffOut.PrepareForOutput( outDimX * outDimY, DeviceTag() );
    WaveletBase::DeviceTranspose( afterY, outDimY, outDimX,
                                  coeffOut, outDimX, outDimY,
                                  0, 0, DeviceTag() );
    return 0;
  }


  // Func:
  // Performs one level of 2D inverse wavelet transform
  // It takes care of boundary conditions.
  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  FLOAT_64 IDWT2Dv2( const ArrayInType                            &coeffIn,
                           std::vector<vtkm::Id>                  &L,
                           ArrayOutType                           &sigOut,
                           DeviceTag                                      )
  {
    VTKM_ASSERT( L.size() == 10 );
    vtkm::Id inDimX = L[0] + L[4];
    vtkm::Id inDimY = L[1] + L[3];
    VTKM_ASSERT( inDimX * inDimY == coeffIn.GetNumberOfValues() );

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    typedef vtkm::worklet::wavelets::InverseTransform2DOdd    IdwtOddWorklet;
    typedef vtkm::worklet::wavelets::InverseTransform2DEven   IdwtEvenWorklet;

    // First inverse transform on columns

    ArrayInType beforeY, beforeYExtend;
    vtkm::Id beforeYDimX = inDimY;
    vtkm::Id beforeYDimY = inDimX;
    beforeY.PrepareForOutput( beforeYDimX * beforeYDimY, DeviceTag() );
    WaveletBase::DeviceTranspose( coeffIn, inDimX, inDimY,
                                  beforeY, beforeYDimX, beforeYDimY,
                                  0, 0, DeviceTag() );
    vtkm::Id cATempLen, beforeYExtendDimX; 
    vtkm::Id beforeYExtendDimY = beforeYDimY;
    this->IDWTHelper( beforeY, L[1], L[3], beforeYDimY, 
                      beforeYExtend, cATempLen, beforeYExtendDimX, 
                      filterLen, wmode, DeviceTag() );
    beforeY.ReleaseResources();
    
    // Inverse transform
    ArrayInType afterY;
    vtkm::Id afterYDimX = beforeYDimX;
    vtkm::Id afterYDimY = beforeYDimY;
    afterY.PrepareForOutput( afterYDimX * afterYDimY, DeviceTag() );
    if( filterLen % 2 != 0 )
    {
      IdwtOddWorklet worklet( filterLen, beforeYExtendDimX, beforeYExtendDimY, 
                              afterYDimX, afterYDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtOddWorklet, DeviceTag> 
          dispatcher( worklet );
      dispatcher.Invoke( beforeYExtend, 
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         afterY );
    }
    else  
    {
      IdwtEvenWorklet worklet( filterLen, beforeYExtendDimX, beforeYExtendDimY, 
                               afterYDimX, afterYDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtEvenWorklet, DeviceTag> 
          dispatcher( worklet );
      dispatcher.Invoke( beforeYExtend, 
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         afterY );
    }

    beforeYExtend.ReleaseResources();

    // Second inverse transform on rows
    ArrayInType beforeX, beforeXExtend;
    vtkm::Id beforeXDimX = afterYDimY;
    vtkm::Id beforeXDimY = afterYDimX;
    vtkm::Id beforeXExtendDimX;
    vtkm::Id beforeXExtendDimY = beforeXDimY;    

    beforeX.PrepareForOutput( beforeXDimX * beforeXDimY, DeviceTag() );
    WaveletBase::DeviceTranspose( afterY, afterYDimX, afterYDimY,
                                  beforeX, beforeXDimX, beforeXDimY,
                                  0, 0, DeviceTag() );
    this->IDWTHelper( beforeX, L[0], L[4], beforeXDimY,
                      beforeXExtend, cATempLen, beforeXExtendDimX,
                      filterLen, wmode, DeviceTag() );
    beforeX.ReleaseResources();

    // Inverse transform
    ArrayInType afterX;
    vtkm::Id afterXDimX = beforeXDimX;
    vtkm::Id afterXDimY = beforeXDimY;
    afterX.PrepareForOutput( afterXDimX * afterXDimY, DeviceTag() );
    if( filterLen % 2 != 0 )
    {
      IdwtOddWorklet worklet( filterLen, beforeXExtendDimX, beforeXExtendDimY, 
                              afterXDimX, afterXDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtOddWorklet, DeviceTag> 
          dispatcher( worklet );
      dispatcher.Invoke( beforeXExtend, 
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         afterX );
    }
    else  
    {
      IdwtEvenWorklet worklet( filterLen, beforeXExtendDimX, beforeXExtendDimY, 
                               afterXDimX, afterXDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtEvenWorklet, DeviceTag> 
          dispatcher( worklet );
      dispatcher.Invoke( beforeXExtend, 
                         WaveletBase::filter.GetLowReconstructFilter(),
                         WaveletBase::filter.GetHighReconstructFilter(),
                         afterX );
    }

    sigOut = afterX;

    return 0;
  }


  // decides the correct extension modes for cA and cD separately,
  // and then extend them to get ready for IDWT
  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  void IDWTHelper( const ArrayInType               &coeffIn,
                         vtkm::Id                  cADimX,           // of codffIn
                         vtkm::Id                  cDDimX,           // of codffIn
                         vtkm::Id                  inDimY,           // of codffIn
                         ArrayOutType              &coeffExtend,
                         vtkm::Id                  &cATempLen,       // output
                         vtkm::Id                  &coeffExtendDimX, // output
                         vtkm::Id                  filterLen, 
                         DWTMode                   mode,  
                         DeviceTag                            )
  {
    vtkm::Id inDimX = cADimX + cDDimX;

    // determine extension modes
    DWTMode cALeft, cARight, cDLeft, cDRight;
    cALeft = cARight = cDLeft = cDRight = mode;
    if( mode == SYMH )
    {   
      cDLeft = ASYMH;
      if( inDimX % 2 != 0 ) 
      {   
        cARight = SYMW;
        cDRight = ASYMW;
      }   
      else
        cDRight = ASYMH;
    }   
    else  // mode == SYMW
    {   
      cDLeft = SYMH;
      if( inDimX % 2 != 0 ) 
      {   
        cARight = SYMW;
        cDRight = SYMH;
      }   
      else
        cARight = SYMH;
    } 
    // determine length after extension
    vtkm::Id cDTempLen;                 // cD dimension after extension. cA is passed in
    vtkm::Id cDPadLen  = 0;
    vtkm::Id addLen = filterLen / 4;    // addLen == 0 for Haar kernel
    if( (cADimX > cDDimX) && (mode == SYMH) )
      cDPadLen = cADimX;
    cATempLen = cADimX + 2 * addLen;
    cDTempLen = cATempLen;

    // extract cA
    vtkm::Id cADimY    = inDimY;
    ArrayInType        cA, cATemp;
    cA.PrepareForOutput( cADimX * cADimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyFrom( cA, cADimX, cADimY,
                                          coeffIn, inDimX, inDimY,
                                          0, 0, DeviceTag() );
    // extend cA
    this->Extend2D( cA, cADimX, cADimY, cATemp, addLen, 
                    cALeft, cARight, false, false, DeviceTag() );
    cA.ReleaseResources();

    // extract cD
    vtkm::Id cDDimY     = inDimY;
    ArrayInType         cD, cDTemp;
    cD.PrepareForOutput( cDDimX * cDDimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyFrom( cD, cDDimX, cDDimY,
                                          coeffIn, inDimX, inDimY,
                                          cADimX, 0, DeviceTag() );
    // extend cD
    if( cDPadLen > 0 )
    {
      this->Extend2D( cD, cDDimX, cDDimY, cDTemp, addLen,
                      cDLeft, cDRight, true, false, DeviceTag() );
    }
    else
    {
      vtkm::Id cDTempLenWouldBe = cDDimX + 2 * addLen;
      if( cDTempLenWouldBe ==  cDTempLen )
        this->Extend2D( cD, cDDimX, cDDimY, cDTemp, addLen, 
                        cDLeft, cDRight, false, false, DeviceTag());
      else if( cDTempLenWouldBe ==  cDTempLen - 1 )
        this->Extend2D( cD, cDDimX, cDDimY, cDTemp, addLen, 
                        cDLeft, cDRight, false, true , DeviceTag());
      else
        vtkm::cont::ErrorControlInternal("cDTemp Length not match!");
    }
    cD.ReleaseResources();

    // combine cATemp and cDTemp
    coeffExtendDimX = cATempLen + cDTempLen;
    vtkm::Id coeffExtendDimY = inDimY;
    coeffExtend.PrepareForOutput( coeffExtendDimX * coeffExtendDimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyTo( cATemp, cATempLen, cADimY,
                                        coeffExtend, coeffExtendDimX, coeffExtendDimY,
                                        0, 0, DeviceTag() );    
    WaveletBase::DeviceRectangleCopyTo( cDTemp, cDTempLen, cDDimY,
                                        coeffExtend, coeffExtendDimX, coeffExtendDimY,
                                        cATempLen, 0, DeviceTag() );
  }

};    

}     // namespace wavelets
}     // namespace worklet
}     // namespace vtkm

#endif 
