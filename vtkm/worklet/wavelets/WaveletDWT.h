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


  template< typename SigInArrayType, typename ExtensionArrayType, typename DeviceTag >
  vtkm::Id Extend2Dv3(const SigInArrayType                     &sigIn,   // Input
                            vtkm::Id                           sigDimX,             
                            vtkm::Id                           sigDimY,
                            ExtensionArrayType                 &ext1,  // left/top extension
                            ExtensionArrayType                 &ext2,  // right/bottom extension
                            vtkm::Id                           addLen,
                            vtkm::worklet::wavelets::DWTMode   ext1Method,
                            vtkm::worklet::wavelets::DWTMode   ext2Method, 
                            bool                               pretendSigPaddedZero, 
                            bool                               padZeroAtExt2,
                            bool                               modeLR,  // true = left-right
                                                                        // false = top-down
                            DeviceTag                                                    )
  {
    // pretendSigPaddedZero and padZeroAtExt2 cannot happen at the same time
    VTKM_ASSERT( !pretendSigPaddedZero || !padZeroAtExt2 );

    if( addLen == 0 )     // Haar kernel
    {
      ext1.PrepareForOutput( 0, DeviceTag() );
      if( pretendSigPaddedZero || padZeroAtExt2 )
      {
        if( modeLR )  // right extension
        {
          ext2.PrepareForOutput( sigDimY, DeviceTag() );
          WaveletBase::DeviceAssignZero2DColumn( ext2, 1, sigDimY, 0, DeviceTag() );
        }
        else          // bottom extension
        {
          ext2.PrepareForOutput( sigDimX, DeviceTag() );
          WaveletBase::DeviceAssignZero2DRow( ext2, sigDimX, 1, 0, DeviceTag() );
        }
      }
      else
        ext2.PrepareForOutput( 0, DeviceTag() );
      return 0;
    }

    typedef typename SigInArrayType::ValueType                 ValueType;
    typedef vtkm::cont::ArrayHandle< ValueType >               ExtendArrayType; 
    typedef vtkm::worklet::wavelets::ExtensionWorklet2D        ExtensionWorklet;
    typedef typename vtkm::worklet::DispatcherMapField< ExtensionWorklet, DeviceTag >
                                                               DispatcherType;
    vtkm::Id extDimX, extDimY;
    vtkm::worklet::wavelets::ExtensionDirection2D dir;

    // Work on left/top extension 
    {
    if( modeLR )
    {
      dir = vtkm::worklet::wavelets::ExtensionDirection2D::LEFT;
      extDimX = addLen;
      extDimY = sigDimY; 
    }
    else
    {
      dir = vtkm::worklet::wavelets::ExtensionDirection2D::TOP;
      extDimX = sigDimX;
      extDimY = addLen; 
    }
    ext1.PrepareForOutput( extDimX * extDimY, DeviceTag() );
    ExtensionWorklet worklet( extDimX, extDimY, sigDimX, sigDimY, ext1Method,
                              dir, false );    // not treating sigIn as having zeros
    DispatcherType dispatcher( worklet );
    dispatcher.Invoke( ext1, sigIn );
    }

    // Work on right/bottom extension
    if( !pretendSigPaddedZero && !padZeroAtExt2 )
    {
      if( modeLR )
      {
        dir = vtkm::worklet::wavelets::ExtensionDirection2D::RIGHT;
        extDimX = addLen;
        extDimY = sigDimY;
      }
      else
      {
        dir = vtkm::worklet::wavelets::ExtensionDirection2D::BOTTOM;
        extDimX = sigDimX;
        extDimY = addLen;
      }
      ext2.PrepareForOutput( extDimX * extDimY, DeviceTag() );
      ExtensionWorklet worklet( extDimX, extDimY, sigDimX, sigDimY, ext2Method,
                                dir, false );
      DispatcherType dispatcher( worklet );
      dispatcher.Invoke( ext2, sigIn );
    }
    else if( !pretendSigPaddedZero && padZeroAtExt2 )
    {
      if( modeLR )
      {
        dir = vtkm::worklet::wavelets::ExtensionDirection2D::RIGHT;
        extDimX = addLen+1;
        extDimY = sigDimY;
      }
      else
      {
        dir = vtkm::worklet::wavelets::ExtensionDirection2D::BOTTOM;
        extDimX = sigDimX;
        extDimY = addLen+1;
      }
      ext2.PrepareForOutput( extDimX * extDimY, DeviceTag() );
      ExtensionWorklet worklet( extDimX, extDimY, sigDimX, sigDimY, ext2Method,
                                dir, false );
      DispatcherType dispatcher( worklet );
      dispatcher.Invoke( ext2, sigIn );
      if( modeLR )
        WaveletBase::DeviceAssignZero2DColumn( ext2, extDimX, extDimY,
                                               extDimX-1, DeviceTag() );
      else
        WaveletBase::DeviceAssignZero2DRow( ext2, extDimX, extDimY,
                                            extDimY-1, DeviceTag() );
    }
    else  // pretendSigPaddedZero
    {
      ExtendArrayType ext2Temp;
      if( modeLR )
      {
        dir = vtkm::worklet::wavelets::ExtensionDirection2D::RIGHT;
        extDimX = addLen;
        extDimY = sigDimY;
      }
      else
      {
        dir = vtkm::worklet::wavelets::ExtensionDirection2D::BOTTOM;
        extDimX = sigDimX;
        extDimY = addLen;
      }
      ext2Temp.PrepareForOutput( extDimX * extDimY, DeviceTag() );
      ExtensionWorklet worklet( extDimX, extDimY, sigDimX, sigDimY, ext2Method,
                                dir, true );    // pretend sig is padded a zero
      DispatcherType dispatcher( worklet );
      dispatcher.Invoke( ext2Temp, sigIn );
      
      if( modeLR )
      {
        ext2.PrepareForOutput( (extDimX+1) * extDimY, DeviceTag() );
        WaveletBase::DeviceRectangleCopyTo( ext2Temp, extDimX, extDimY,
                                            ext2, extDimX+1, extDimY,
                                            1, 0, DeviceTag() );
        WaveletBase::DeviceAssignZero2DColumn( ext2, extDimX+1, extDimY,
                                               0, DeviceTag() );
      }
      else
      {
        ext2.PrepareForOutput( extDimX * (extDimY+1), DeviceTag() );
        WaveletBase::DeviceRectangleCopyTo( ext2Temp, extDimX, extDimY,
                                            ext2, extDimX, extDimY+1,
                                            0, 1, DeviceTag() );
        WaveletBase::DeviceAssignZero2DRow( ext2, extDimX, extDimY+1, 
                                            0, DeviceTag() );
      }
    }

#if 0
    {
      vtkm::Id extDimXRight = extDimX;
      if( attachZeroRightRight )
        extDimXRight++;

      rightExt.PrepareForOutput( extDimXRight * extDimY, DeviceTag() );
  
      ExtensionWorklet worklet( extDimXRight, extDimY, sigDimX, sigDimY, rightExtMethod,
                                vtkm::worklet::wavelets::ExtensionDirection2D::RIGHT,
                                false );
      vtkm::worklet::DispatcherMapField< ExtensionWorklet, DeviceTag >
          dispatcher( worklet );
      dispatcher.Invoke( rightExt, sigIn );

      if( attachZeroRightRight )
        WaveletBase::DeviceAssignZero2DColumn( rightExt, extDimXRight, extDimY,
                                               extDimXRight-1, DeviceTag() );
    }
    else    // attachZeroRightLeft mode. 
    {
      ExtendArrayType rightExtendTmp;
      rightExtendTmp.PrepareForOutput( extDimX * extDimY, DeviceTag() );

      ExtensionWorklet worklet( extDimX, extDimY, sigDimX, sigDimY, rightExtMethod,
                                vtkm::worklet::wavelets::ExtensionDirection2D::RIGHT,
                                true );     // treat sigIn as having zeros
      vtkm::worklet::DispatcherMapField< ExtensionWorklet, DeviceTag >
          dispatcher( worklet );
      dispatcher.Invoke( rightExt, sigIn );

      rightExt.PrepareForOutput( (extDimX+1) * extDimY, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( rightExtendTmp, extDimX,   extDimY,
                                          rightExt,    extDimX+1, extDimY,
                                          1, 0, DeviceTag() );
      WaveletBase::DeviceAssignZero2DColumn( rightExt, extDimX+1, extDimY,
                                             0, DeviceTag() );
    }
#endif  
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
    typedef vtkm::cont::ArrayHandleConcatenate< ExtensionArrayType, SigInArrayType> 
            ArrayConcat;
    
    ExtensionArrayType                              leftExtend, rightExtend;

    if( addLen == 0 )   // Haar kernel
    {
      if( attachZeroRightLeft || attachZeroRightRight )
      {
        leftExtend.PrepareForOutput( 0, DeviceTag() );
        rightExtend.PrepareForOutput(1, DeviceTag() );
        WaveletBase::DeviceAssignZero( rightExtend, 0, DeviceTag() );
      }
      else
      {
        leftExtend.PrepareForOutput( 0, DeviceTag() );
        rightExtend.PrepareForOutput(0, DeviceTag() );
      }
      ArrayConcat leftOn( leftExtend, sigIn );    
      sigOut = vtkm::cont::make_ArrayHandleConcatenate( leftOn, rightExtend );
      return 0;
    }

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
    vtkm::worklet::wavelets::ForwardTransform<DeviceTag> forwardTransform 
          ( WaveletBase::filter.GetLowDecomposeFilter(),
            WaveletBase::filter.GetHighDecomposeFilter(),
            filterLen, L[0], L[1], oddLow, oddHigh );

    coeffOut.PrepareForOutput( sigExtendedLen, DeviceTag() );
    vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::ForwardTransform<DeviceTag>, DeviceTag> 
        dispatcher(forwardTransform);
    // put a timer
    vtkm::cont::Timer<DeviceTag> timer;
    dispatcher.Invoke( sigInExtended, 
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
    else    
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
  std::cerr << std::endl;
}

    // Allocate memory for sigOut
    sigOut.PrepareForOutput( cATempLen + cDTempLen, DeviceTag() );

    vtkm::Float64 elapsedTime = 0; 
    if( filterLen % 2 != 0 )
    {
      vtkm::worklet::wavelets::InverseTransformOdd<DeviceTag> inverseXformOdd
          ( WaveletBase::filter.GetLowReconstructFilter(),
            WaveletBase::filter.GetHighReconstructFilter(), 
            filterLen, L[0], cATempLen );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::
            InverseTransformOdd<DeviceTag>, DeviceTag> dispatcher( inverseXformOdd );
      // use a timer
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke( coeffInExtended, sigOut );
      elapsedTime = timer.GetElapsedTime();
    }
    else
    {
      vtkm::worklet::wavelets::InverseTransformEven<DeviceTag> inverseXformEven
            ( WaveletBase::filter.GetLowReconstructFilter(),
              WaveletBase::filter.GetHighReconstructFilter(),
              filterLen, L[0], cATempLen, !doSymConv );
      vtkm::worklet::DispatcherMapField<vtkm::worklet::wavelets::
            InverseTransformEven<DeviceTag>, DeviceTag> dispatcher( inverseXformEven );
      // use a timer
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke( coeffInExtended, sigOut );
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


  // Performs one level of 2D discrete wavelet transform
  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  FLOAT_64 DWT2Dv3( const ArrayInType                 &sigIn,
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
    typedef vtkm::worklet::wavelets::ForwardTransform2Dv3<DeviceTag> ForwardXFormv3;
    typedef vtkm::worklet::wavelets::ForwardTransform2D<DeviceTag> ForwardXForm;
    typedef typename vtkm::worklet::DispatcherMapField< ForwardXFormv3, DeviceTag > 
            DispatcherType;

    ArrayType   sigExtended;
    vtkm::Id outDimX = sigDimX;
    vtkm::Id outDimY = sigDimY;

    // First transform on rows
    ArrayType         afterX;
    afterX.PrepareForOutput( sigDimX * sigDimY, DeviceTag() );
    {
      ArrayType         leftExt, rightExt;
      this->Extend2Dv3( sigIn, sigDimX, sigDimY, leftExt, rightExt, addLen, 
                       WaveletBase::wmode, WaveletBase::wmode, false, false, 
                       true, DeviceTag() );  // Extend in left-right direction
      ForwardXFormv3 worklet( WaveletBase::filter.GetLowDecomposeFilter(),
                              WaveletBase::filter.GetHighDecomposeFilter(),
                              filterLen, L[0], oddLow, true, // left-right
                              addLen, sigDimY, sigDimX, sigDimY, addLen, sigDimY );
      DispatcherType dispatcher(worklet);
      dispatcher.Invoke( leftExt, sigIn, rightExt, afterX );
//WaveletBase::Print2DArray("\ntest results afrer DWT on rows:", afterX, sigDimX ); 
    }

    // Then do transform in Y direction
    coeffOut.PrepareForOutput( sigDimX * sigDimY, DeviceTag() );
    {
      ArrayType         topExt, bottomExt;
      this->Extend2Dv3( afterX, sigDimX, sigDimY, topExt, bottomExt, addLen,
                       WaveletBase::wmode, WaveletBase::wmode, false, false, 
                       false, DeviceTag() );   // Extend in top-down direction
//WaveletBase::Print2DArray("top ext:", topExt, sigDimX );
//WaveletBase::Print2DArray("signal:", afterX, sigDimX );
//WaveletBase::Print2DArray("bottom ext:", bottomExt, sigDimX );
      ForwardXFormv3 worklet( WaveletBase::filter.GetLowDecomposeFilter(),
                              WaveletBase::filter.GetHighDecomposeFilter(),
                              filterLen, L[1], oddLow, false, // top-down
                              sigDimX, addLen, sigDimX, sigDimY, sigDimX, addLen );
      DispatcherType dispatcher( worklet );
      dispatcher.Invoke( topExt, afterX, bottomExt, coeffOut );
    }
    return 0;
  }


  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  FLOAT_64 IDWT2Dv3( const ArrayInType                            &coeffIn,
                     const std::vector<vtkm::Id>                  &L,
                           ArrayOutType                           &sigOut,
                           DeviceTag                                      )
  {
    VTKM_ASSERT( L.size() == 10 );
    vtkm::Id inDimX = L[0] + L[4];
    vtkm::Id inDimY = L[1] + L[3];
    VTKM_ASSERT( inDimX * inDimY == coeffIn.GetNumberOfValues() );

    vtkm::Id filterLen = WaveletBase::filter.GetFilterLength();
    typedef vtkm::cont::ArrayHandle<typename ArrayInType::ValueType>     BasicArrayType;
    typedef vtkm::worklet::wavelets::InverseTransform2D<DeviceTag>       IDWT2DWorklet;
    typedef vtkm::worklet::DispatcherMapField<IDWT2DWorklet, DeviceTag>  Dispatcher;

    // First inverse transform on columns
    BasicArrayType        ext1, ext2, ext3, ext4;
    vtkm::Id              extDimX = inDimX; 
    vtkm::Id              ext1DimY, ext2DimY, ext3DimY, ext4DimY;
    this->IDWTHelperTD( coeffIn, L[1], L[3], inDimX, ext1, ext2, ext3, ext4,
                        ext1DimY, ext2DimY, ext3DimY, ext4DimY,
                        filterLen, wmode, DeviceTag() );
    BasicArrayType        afterY;
    afterY.PrepareForOutput( inDimX * inDimY, DeviceTag() );
    {
    IDWT2DWorklet worklet( WaveletBase::filter.GetLowReconstructFilter(),
                           WaveletBase::filter.GetHighReconstructFilter(),
                           filterLen,
                           extDimX, ext1DimY,     // ext1
                           inDimX,  L[1],         // cA
                           extDimX, ext2DimY,     // ext2
                           extDimX, ext3DimY,     // ext3
                           inDimX,  L[3],         // cD
                           extDimX, ext4DimY,     // ext4
                           false );               // top-down
    Dispatcher dispatcher( worklet );
    dispatcher.Invoke( ext1, ext2, ext3, ext4, coeffIn, afterY );
    }
WaveletBase::Print2DArray("\ntest afterY:", afterY, inDimX );
    
    // Then inverse transform on rows
    vtkm::Id extDimY = inDimY;
    vtkm::Id ext1DimX, ext2DimX, ext3DimX, ext4DimX;
    this->IDWTHelperLR( afterY, L[0], L[4], inDimY, ext1, ext2, ext3, ext4,
                        ext1DimX, ext2DimX, ext3DimX, ext4DimX,
                        filterLen, wmode, DeviceTag() ); 
    sigOut.PrepareForOutput( inDimX * inDimY, DeviceTag() );
std::cerr << "start calling worklet" << std::endl;
    {
      IDWT2DWorklet worklet( WaveletBase::filter.GetLowReconstructFilter(),
                             WaveletBase::filter.GetHighReconstructFilter(),
                             filterLen,
                             ext1DimX, extDimY,   // ext1
                             L[0],     inDimY,    // cA
                             ext2DimX, extDimY,   // ext2
                             ext3DimX, extDimY,   // ext3
                             L[4],     inDimY,    // cA
                             ext4DimX, extDimY,   // ext4
                             true );              // left-right
      Dispatcher dispatcher( worklet );
      dispatcher.Invoke( ext1, ext2, ext3, ext4, afterY, sigOut );
    }
std::cerr << "end calling worklet" << std::endl;
ext1.ReleaseResources();
std::cerr << "ext1 released" << std::endl;
ext2.ReleaseResources();
std::cerr << "ext2 released" << std::endl;
ext3.ReleaseResources();
std::cerr << "ext3 released" << std::endl;
ext4.ReleaseResources();
std::cerr << "ext4 released" << std::endl;
afterY.ReleaseResources();
std::cerr << "afterY released" << std::endl;

    return 0;
  }


  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions.
  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  void IDWTHelperLR( const ArrayInType             &coeffIn,
                         vtkm::Id                  cADimX,     // of codffIn
                         vtkm::Id                  cDDimX,     // of codffIn
                         vtkm::Id                  inDimY,     // of codffIn
                         ArrayOutType              &ext1,      // output
                         ArrayOutType              &ext2,      // output
                         ArrayOutType              &ext3,      // output
                         ArrayOutType              &ext4,      // output
                         vtkm::Id                  &ext1DimX,  // output
                         vtkm::Id                  &ext2DimX,  // output
                         vtkm::Id                  &ext3DimX,  // output
                         vtkm::Id                  &ext4DimX,  // output
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
    vtkm::Id cAExtendedDimX, cDExtendedDimX;
    vtkm::Id cDPadLen  = 0;
    vtkm::Id addLen = filterLen / 4;    // addLen == 0 for Haar kernel
    if( (cADimX > cDDimX) && (mode == SYMH) )
      cDPadLen = cADimX;
    cAExtendedDimX = cADimX + 2 * addLen;
    cDExtendedDimX = cAExtendedDimX;
    typedef vtkm::cont::ArrayHandle<typename ArrayInType::ValueType>
            BasicArrayType;

    // extract cA
    vtkm::Id cADimY    = inDimY;
    BasicArrayType     cA;
    cA.PrepareForOutput( cADimX * cADimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyFrom( cA, cADimX, cADimY,
                                          coeffIn, inDimX, inDimY,
                                          0, 0, DeviceTag() );
    // extend cA
    this->Extend2Dv3( cA, cADimX, cADimY, ext1, ext2, addLen, 
                    cALeft, cARight, false, false, true, DeviceTag() );
    cA.ReleaseResources();
    ext1DimX = ext2DimX = addLen;

    // extract cD
    vtkm::Id cDDimY     = inDimY;
    BasicArrayType      cD;
    cD.PrepareForOutput( cDDimX * cDDimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyFrom( cD, cDDimX, cDDimY,
                                          coeffIn, inDimX, inDimY,
                                          cADimX, 0, DeviceTag() );
    // extend cD
    if( cDPadLen > 0 )
    {
      this->Extend2Dv3( cD, cDDimX, cDDimY, ext3, ext4, addLen,
                      cDLeft, cDRight, true, false, true, DeviceTag() );
      ext3DimX = addLen;
      ext4DimX = addLen + 1;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimX + 2 * addLen;
      if( cDExtendedWouldBe ==  cDExtendedDimX )
      {
        this->Extend2Dv3( cD, cDDimX, cDDimY, ext3, ext4, addLen, 
                          cDLeft, cDRight, false, false, true, DeviceTag());
        ext3DimX = ext4DimX = addLen;
      }
      else if( cDExtendedWouldBe ==  cDExtendedDimX - 1 )
      {
        this->Extend2Dv3( cD, cDDimX, cDDimY, ext3, ext4, addLen, 
                          cDLeft, cDRight, false, true, true, DeviceTag());
        ext3DimX = addLen;
        ext4DimX = addLen + 1;
      }
      else
        vtkm::cont::ErrorControlInternal("cDTemp Length not match!");
    }
  }


  // decides the correct extension modes for cA and cD separately,
  // and fill the extensions.
  template< typename ArrayInType, typename ArrayOutType, typename DeviceTag >
  void IDWTHelperTD( const ArrayInType             &coeffIn,
                         vtkm::Id                  cADimY,     // of codffIn
                         vtkm::Id                  cDDimY,     // of codffIn
                         vtkm::Id                  inDimX,     // of codffIn
                         ArrayOutType              &ext1,      // output
                         ArrayOutType              &ext2,      // output
                         ArrayOutType              &ext3,      // output
                         ArrayOutType              &ext4,      // output
                         vtkm::Id                  &ext1DimY,  // output
                         vtkm::Id                  &ext2DimY,  // output
                         vtkm::Id                  &ext3DimY,  // output
                         vtkm::Id                  &ext4DimY,  // output
                         vtkm::Id                  filterLen, 
                         DWTMode                   mode,
                         DeviceTag                            )
  {
    vtkm::Id inDimY = cADimY + cDDimY;

    // determine extension modes
    DWTMode cATop, cABottom, cDTop, cDBottom;
    cATop = cABottom = cDTop = cDBottom = mode;
    if( mode == SYMH )
    {   
      cDTop = ASYMH;
      if( inDimY % 2 != 0 ) 
      {   
        cABottom = SYMW;
        cDBottom = ASYMW;
      }   
      else
        cDBottom = ASYMH;
    }   
    else  // mode == SYMW
    {   
      cDTop = SYMH;
      if( inDimY % 2 != 0 ) 
      {   
        cABottom = SYMW;
        cDBottom = SYMH;
      }   
      else
        cABottom = SYMH;
    } 
    // determine length after extension
    vtkm::Id cAExtendedDimY, cDExtendedDimY;
    vtkm::Id cDPadLen  = 0;
    vtkm::Id addLen = filterLen / 4;    // addLen == 0 for Haar kernel
    if( (cADimY > cDDimY) && (mode == SYMH) )
      cDPadLen = cADimY;
    cAExtendedDimY = cADimY + 2 * addLen;
    cDExtendedDimY = cAExtendedDimY;
    typedef vtkm::cont::ArrayHandle<typename ArrayInType::ValueType>
            BasicArrayType;

    // extract cA
    vtkm::Id cADimX    = inDimX;
    BasicArrayType     cA;
    cA.PrepareForOutput( cADimX * cADimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyFrom( cA, cADimX, cADimY,
                                          coeffIn, inDimX, inDimY,
                                          0, 0, DeviceTag() );
    // extend cA
    this->Extend2Dv3( cA, cADimX, cADimY, ext1, ext2, addLen, 
                      cATop, cABottom, false, false, false, DeviceTag() );
    cA.ReleaseResources();
    ext1DimY = ext2DimY = addLen;

    // extract cD
    vtkm::Id cDDimX     = inDimX;
    BasicArrayType      cD;
    cD.PrepareForOutput( cDDimX * cDDimY, DeviceTag() );
    WaveletBase::DeviceRectangleCopyFrom( cD, cDDimX, cDDimY,
                                          coeffIn, inDimX, inDimY,
                                          0, cADimY, DeviceTag() );
    // extend cD
    if( cDPadLen > 0 )
    {
      this->Extend2Dv3( cD, cDDimX, cDDimY, ext3, ext4, addLen,
                        cDTop, cDBottom, true, false, false, DeviceTag() );
      ext3DimY = addLen;
      ext4DimY = addLen + 1;
    }
    else
    {
      vtkm::Id cDExtendedWouldBe = cDDimY + 2 * addLen;
      if( cDExtendedWouldBe ==  cDExtendedDimY )
      {
        this->Extend2Dv3( cD, cDDimX, cDDimY, ext3, ext4, addLen, 
                          cDTop, cDBottom, false, false, false, DeviceTag());
        ext3DimY = ext4DimY = addLen;
      }
      else if( cDExtendedWouldBe ==  cDExtendedDimY - 1 )
      {
        this->Extend2Dv3( cD, cDDimX, cDDimY, ext3, ext4, addLen, 
                          cDTop, cDBottom, false, true, false, DeviceTag());
        ext3DimY = addLen;
        ext4DimY = addLen + 1;
      }
      else
        vtkm::cont::ErrorControlInternal("cDTemp Length not match!");
    }
  }


  /*
   * old implementations *
   */


  // Performs one level of 2D discrete wavelet transform
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

    vtkm::cont::Timer<DeviceTag> timer;
    vtkm::Float64 elapsedTime = 0.0;
    typedef vtkm::worklet::wavelets::ForwardTransform2D<DeviceTag> ForwardXForm;
    {
    ForwardXForm worklet( WaveletBase::filter.GetLowDecomposeFilter(),
                          WaveletBase::filter.GetHighDecomposeFilter(),
                          filterLen, L[0], sigExtendedDimX, sigExtendedDimY,
                          outDimX, outDimY, oddLow );
    vtkm::worklet::DispatcherMapField< ForwardXForm, DeviceTag > dispatcher( worklet );
    timer.Reset();
    dispatcher.Invoke( sigExtended, afterX );
    elapsedTime += timer.GetElapsedTime();
//WaveletBase::Print2DArray("\ntrue results afrer DWT on rows: ", afterX, sigDimX );
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
//WaveletBase::Print2DArray( "\ntrue results after extension on columns",
//                           sigExtended, outDimX );
    ArrayType   afterY;
    afterY.PrepareForOutput( outDimY * outDimX, DeviceTag() );
    {
    ForwardXForm worklet2( WaveletBase::filter.GetLowDecomposeFilter(),
                           WaveletBase::filter.GetHighDecomposeFilter(),
                           filterLen, L[1], sigExtendedDimX, sigExtendedDimY,
                           outDimY, outDimX, oddLow );
    vtkm::worklet::DispatcherMapField< ForwardXForm, DeviceTag > dispatcher2( worklet2 );
    timer.Reset();
    dispatcher2.Invoke( sigExtended, afterY );
    elapsedTime += timer.GetElapsedTime();
    }
    sigExtended.ReleaseResources();

    // Transpose to output
    coeffOut.PrepareForOutput( outDimX * outDimY, DeviceTag() );
    WaveletBase::DeviceTranspose( afterY, outDimY, outDimX,
                                  coeffOut, outDimX, outDimY,
                                  0, 0, DeviceTag() );
    return elapsedTime;
  }


  // Performs one level of 2D inverse wavelet transform
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
    typedef vtkm::worklet::wavelets::InverseTransform2DOdd<DeviceTag>    IdwtOddWorklet;
    typedef vtkm::worklet::wavelets::InverseTransform2DEven<DeviceTag>   IdwtEvenWorklet;
    vtkm::Float64   elapsedTime = 0.0;

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

//WaveletBase::Print2DArrayTransposed("\ntrue results afrer extension on columns (transposed): ", 
//             beforeYExtend, beforeYExtendDimX, beforeYExtendDimY );
    
    ArrayInType afterY;
    vtkm::Id afterYDimX = beforeYDimX;
    vtkm::Id afterYDimY = beforeYDimY;
    afterY.PrepareForOutput( afterYDimX * afterYDimY, DeviceTag() );
    if( filterLen % 2 != 0 )
    {
      IdwtOddWorklet worklet( WaveletBase::filter.GetLowReconstructFilter(),
                              WaveletBase::filter.GetHighReconstructFilter(),
                              filterLen, beforeYExtendDimX, beforeYExtendDimY, 
                              afterYDimX, afterYDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtOddWorklet, DeviceTag> 
          dispatcher( worklet );
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke( beforeYExtend, afterY );
      elapsedTime += timer.GetElapsedTime();
    }
    else  
    {
      IdwtEvenWorklet worklet( WaveletBase::filter.GetLowReconstructFilter(),
                               WaveletBase::filter.GetHighReconstructFilter(),
                               filterLen, beforeYExtendDimX, beforeYExtendDimY, 
                               afterYDimX, afterYDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtEvenWorklet, DeviceTag> 
          dispatcher( worklet );
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke( beforeYExtend, afterY );
      elapsedTime += timer.GetElapsedTime();
    }

//WaveletBase::Print2DArrayTransposed("\ntrue afterY:", afterY, afterYDimX, afterYDimY);

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

    ArrayInType afterX;
    vtkm::Id afterXDimX = beforeXDimX;
    vtkm::Id afterXDimY = beforeXDimY;
    afterX.PrepareForOutput( afterXDimX * afterXDimY, DeviceTag() );
    if( filterLen % 2 != 0 )
    {
      IdwtOddWorklet worklet( WaveletBase::filter.GetLowReconstructFilter(),
                              WaveletBase::filter.GetHighReconstructFilter(),
                              filterLen, beforeXExtendDimX, beforeXExtendDimY, 
                              afterXDimX, afterXDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtOddWorklet, DeviceTag> 
          dispatcher( worklet );
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke( beforeXExtend, afterX );
      elapsedTime += timer.GetElapsedTime();
    }
    else  
    {
      IdwtEvenWorklet worklet( WaveletBase::filter.GetLowReconstructFilter(),
                               WaveletBase::filter.GetHighReconstructFilter(),
                               filterLen, beforeXExtendDimX, beforeXExtendDimY, 
                               afterXDimX, afterXDimY, cATempLen );
      vtkm::worklet::DispatcherMapField<IdwtEvenWorklet, DeviceTag> 
          dispatcher( worklet );
      vtkm::cont::Timer<DeviceTag> timer;
      dispatcher.Invoke( beforeXExtend, afterX );
      elapsedTime += timer.GetElapsedTime();
    }

    sigOut = afterX;

    return elapsedTime;
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


  template< typename SigInArrayType, typename SigExtendedArrayType, typename DeviceTag >
  vtkm::Id Extend2D(const SigInArrayType                           &sigIn,   // Input
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
    if( addLen == 0 )     // Haar kernel
    {
      if( attachZeroRightLeft || attachZeroRightRight )
      {
        sigOut.PrepareForOutput( (sigDimX+1) * sigDimY, DeviceTag() );
        WaveletBase::DeviceRectangleCopyTo( sigIn,  sigDimX,   sigDimY,
                                            sigOut, sigDimX+1, sigDimY,
                                            0, 0, DeviceTag() );
        WaveletBase::DeviceAssignZero2DColumn( sigOut, sigDimX+1, sigDimY,
                                               sigDimX, DeviceTag() );
      }
      else
      {
        sigOut.PrepareForOutput( sigDimX * sigDimY, DeviceTag() );
        vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>::Copy( sigIn, sigOut );
      }
      return 0;
    }

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
    typedef vtkm::worklet::wavelets::LeftExtensionWorklet2D  LeftWorkletType;
    LeftWorkletType leftWorklet( extDimX, extDimY, sigDimX, sigDimY, leftExtMethod );
    vtkm::worklet::DispatcherMapField< LeftWorkletType, DeviceTag > 
          dispatcher( leftWorklet );
    dispatcher.Invoke( leftExtend, sigIn );

    // Work on right extension
    typedef vtkm::worklet::wavelets::RightExtensionWorklet2D    RightWorkletType;
    ExtendArrayType rightExtend;
    if( !attachZeroRightLeft ) // no attach zero, or only attach on RightRight
    {
      vtkm::Id extDimXRight = extDimX;
      if( attachZeroRightRight )
        extDimXRight++;

      // Allocate memory
      rightExtend.PrepareForOutput( extDimXRight * extDimY, DeviceTag() );
  
      RightWorkletType rightWorklet(false, extDimXRight, extDimY, sigDimX, sigDimY, rightExtMethod);
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
      // Allocate memory for rightExtend
      rightExtend.PrepareForOutput( extDimX * extDimY, DeviceTag() );

      // Do the extension
      RightWorkletType rightWorklet( true, extDimX, extDimY, sigDimX, sigDimY, rightExtMethod );
      vtkm::worklet::DispatcherMapField< RightWorkletType, DeviceTag > 
            dispatcher5( rightWorklet );
      dispatcher5.Invoke( rightExtend, sigIn );

      // Allocate memory for output
      sigOut.PrepareForOutput( outputDimX * outputDimY, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( sigIn, sigDimX, sigDimY,
                                          sigOut, outputDimX, outputDimY,
                                          extDimX, 0, DeviceTag() );
      WaveletBase::DeviceAssignZero2DColumn( sigOut, outputDimX, outputDimY,
                                             extDimX+sigDimX, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( leftExtend, extDimX, extDimY,
                                          sigOut, outputDimX, outputDimY,
                                          0, 0, DeviceTag() );
      WaveletBase::DeviceRectangleCopyTo( rightExtend, extDimX, extDimY,
                                          sigOut, outputDimX, outputDimY,
                                          extDimX+sigDimX+1, 0, DeviceTag() );
    }
    return 0;
  }


};    

}     // namespace wavelets
}     // namespace worklet
}     // namespace vtkm

#endif 
