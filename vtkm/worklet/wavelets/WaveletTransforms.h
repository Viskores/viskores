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

#ifndef vtk_m_worklet_Wavelets_h
#define vtk_m_worklet_Wavelets_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {
namespace wavelets {

enum DWTMode {    // boundary extension modes
  SYMH, 
  SYMW,
  ASYMH, 
  ASYMW
};

enum ExtensionDirection2D {  // which side of a matrix to extend
  LEFT,
  RIGHT,
  TOP,
  BOTTOM
};

class IndexTranslator3Matrices
{
public:
  IndexTranslator3Matrices( vtkm::Id x_1, vtkm::Id x_2, vtkm::Id x_3, 
                            vtkm::Id y_1, vtkm::Id y_2, vtkm::Id y_3, bool mode ) :
                            x1(x_1), x2(x_2), x3(x_3), 
                            y1(y_1), y2(y_2), y3(y_3), mode_lr(mode)  {}

  void Translate2Dto1D( vtkm::Id  inX,  vtkm::Id  inY,  // 2D indices as input
                        vtkm::Id  &mat, vtkm::Id  &idx )  // which matrix, and idx of that matrix
  {
    if( mode_lr )   // left-right mode
    {
      if( inX < 0 )
        vtkm::cont::ErrorControlInternal("Invalid index!");
      else if ( 0 <= inX && inX < x1 )
      {
        mat = 0;
        idx = inY * x1 + inX;
      } 
      else if ( x1 <= inX && inX < (x1 + x2) )
      {
        mat = 1;
        idx = inY * x2 + (inX - x1);
      }
      else if ( (x1 + x2) <= inX && inX < (x1 + x2 + x3) )
      {
        mat = 2;  
        idx = inY * x3 + (inX - x1 - x2);
      }
      else
        vtkm::cont::ErrorControlInternal("Invalid index!");
    }
    else          // top-down mode
    {
      if( inY < 0 )
        vtkm::cont::ErrorControlInternal("Invalid index!");
      else if ( 0 <= inY && inY < y1 )
      {
        mat = 0;
        idx = inY * x1 + inX;
      }
      else if ( y1 <= inY && inY < (y1 + y2) )
      {
        mat = 1;
        idx = (inY - y1) * x1 + inX;
      }
      else if ( (y1 + y2) <= inY && inY < (y1 + y2 + y3) )
      {
        mat = 2;
        idx = (inY - y1 - y2) * x1 + inX;
      }
      else
        vtkm::cont::ErrorControlInternal("Invalid index!");
    }
  }

private:
  const vtkm::Id      x1, x2, x3, y1, y2, y3;         // dimensions of 3 matrices.
  const bool          mode_lr;     // true: left right mode; false: top down mode.
};


// Worklet: perform a simple 2D forward transform
template< typename DeviceTag >
class ForwardTransform2Dv2: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // left/top extension
                                WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayIn<ScalarAll>,     // right/bottom extension
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _4   InputDomain;


  // Constructor
  VTKM_EXEC_CONT_EXPORT
  ForwardTransform2Dv2( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                        const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                        vtkm::Id filter_len, vtkm::Id approx_len, 
                        bool odd_low, bool mode_lr,
                        vtkm::Id x1, vtkm::Id y1, vtkm::Id x2, vtkm::Id y2,
                        vtkm::Id x3, vtkm::Id y3 )
                   :    lowFilter(  loFilter.PrepareForInput( DeviceTag() ) ),
                        highFilter( hiFilter.PrepareForInput( DeviceTag() ) ),
                        filterLen(  filter_len ), approxLen(  approx_len ),
                        outDimX( x2 ), outDimY( y2 ),
                        oddlow( odd_low ), modeLR( mode_lr ),
                        translator( x1, x2, x3, y1, y2, y3, mode_lr )
  { this->SetStartPosition(); }

  VTKM_EXEC_CONT_EXPORT
  void Output1Dto2D( vtkm::Id idx, vtkm::Id &x, vtkm::Id &y ) const     
  {
    x = idx % outDimX;
    y = idx / outDimX;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Output2Dto1D( vtkm::Id x, vtkm::Id y ) const     
  {
    return y * outDimX + x;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InPortalType1, typename InPortalType2, typename InPortalType3 >
  VTKM_EXEC_CONT_EXPORT
  VAL GetVal( const InPortalType1 &portal1, const InPortalType2 &portal2,
              const InPortalType3 &portal3, vtkm::Id inMatrix, vtkm::Id inIdx )
  {
    if( inMatrix == 1 )
      return MAKEVAL( portal1.Get(inIdx) );
    else if( inMatrix == 2 )
      return MAKEVAL( portal2.Get(inIdx) );
    else if( inMatrix == 3 )
      return MAKEVAL( portal3.Get(inIdx) );
    else
        vtkm::cont::ErrorControlInternal("Invalid matrix index!");
  }
  
  template <typename InPortalType1, typename InPortalType2, 
            typename InPortalType3, typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InPortalType1       &inPortal1, // left/top extension
                  const InPortalType2       &inPortal2, // signal
                  const InPortalType3       &inPortal3, // right/bottom extension
                     OutputPortalType       &coeffOut,
                  const vtkm::Id            &workIndex) const
  {
    vtkm::Id outputX, outputY, output1D;
    Output1Dto2D( workIndex, outputX, outputY );
    vtkm::Id inputX = outputX; 
    vtkm::Id inputY = outputY;
    vtkm::Id inputMatrix, inputIdx;
    typedef typename OutputPortalType::ValueType OutputValueType;
    
    if( modeLR )
    {
      if( inputX % 2 == 0 )  // calculate cA
      {
        vtkm::Id xl = xlstart + inputX;
        VAL sum = MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k > -1; k-- )
        {
          translator.Translate2Dto1D( xl, inputY, inputMatrix, inputIdx );
          sum += lowFilter.Get(k) * 
                 GetVal( inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx );
          xl++;
        }
        output1D = Output2Dto1D( inputX/2, outputY );
        coeffOut.Set( output1D, static_cast<OutputValueType>(sum) );
      }
      else                      // calculate cD
      {
        vtkm::Id xh = xhstart + inputX - 1;
        VAL sum=MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k > -1; k-- )
        {
          translator.Translate2Dto1D( xh, inputY, inputMatrix, inputIdx );
          sum += highFilter.Get(k) * 
                 GetVal( inPortal1, inPortal2, inPortal3, inputMatrix, inputIdx );
          xh++;
        }
        output1D = Output2Dto1D( (inputX-1)/2 + approxLen, outputY );
        coeffOut.Set( output1D, static_cast<OutputValueType>(sum) );
      }
    }
  }

  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id outDimX, outDimY;
  bool  oddlow;
  bool  modeLR;             // true = left right; false = top down.
  IndexTranslator3Matrices  translator;
  vtkm::Id xlstart, xhstart;
  
  VTKM_EXEC_CONT_EXPORT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = 1;
  }
};


// Worklet for 2D signal extension
class ExtensionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;
  typedef vtkm::Id Id;

  // Constructor
  VTKM_EXEC_CONT_EXPORT 
  ExtensionWorklet2D( Id x1, Id y1, Id x2, Id y2, DWTMode m,
                      ExtensionDirection2D dir, bool pad_zero)
                    : extDimX( x1 ), extDimY( y1 ), sigDimX( x2 ), sigDimY( y2 ), 
                      mode(m), direction( dir ), padZero( pad_zero )  {}

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  void Ext1Dto2D ( Id idx, Id &x, Id &y ) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  Id Sig2Dto1D( Id x, Id y ) const
  {
    return y * sigDimX + x;
  }

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    Id extX, extY, sigX, sigY;
    Ext1Dto2D( workIndex, extX, extY );
    typename PortalOutType::ValueType sym = 1.0;
    if( mode == ASYMH || mode == ASYMW )
      sym = -1.0;
    if( direction == LEFT )     
    {
      sigY = extY;
      if( mode == SYMH || mode == ASYMH )
        sigX = extDimX - extX - 1;
      else    // mode == SYMW || mode == ASYMW
        sigX = extDimX - extX; 
    }
    else if( direction == TOP ) 
    {
      sigX = extX;
      if( mode == SYMH || mode == ASYMH )
        sigY = extDimY - extY - 1;
      else    // mode == SYMW || mode == ASYMW
        sigY = extDimY - extY; 
    }
    else if( direction == RIGHT )
    {
      sigY = extY;
      if( mode == SYMH || mode == ASYMH )
        sigX = sigDimX - extX - 1;
      else
        sigX = sigDimX - extX - 2;
      if( padZero )
        sigX++;
    }
    else  // direction == BOTTOM 
    {
      sigX = extX;
      if( mode == SYMH || mode == ASYMH )
        sigY = sigDimY - extY - 1;
      else
        sigY = sigDimY - extY - 2;
      if( padZero )
        sigY++;
    }
    if( sigX == sigDimX || sigY == sigDimY )
      portalOut.Set( workIndex, 0.0 );
    else
      portalOut.Set( workIndex, sym * portalIn.Get( Sig2Dto1D(sigX, sigY) ) );
  }

private:
  const vtkm::Id              extDimX, extDimY, sigDimX, sigDimY;
  const ExtensionDirection2D  direction;
  const DWTMode               mode;
  const bool                  padZero;  // only applicable when direction is right or bottom.
};


// Worklet: perform a simple forward transform
template< typename DeviceTag >
class ForwardTransform: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  ForwardTransform( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                    const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                    vtkm::Id filLen, vtkm::Id approx_len, vtkm::Id detail_len,
                    bool odd_low, bool odd_high )  :
                    lowFilter(  loFilter.PrepareForInput(DeviceTag()) ), 
                    highFilter( hiFilter.PrepareForInput(DeviceTag()) ), 
                    filterLen( filLen ), 
                    approxLen( approx_len ), 
                    detailLen( detail_len ),
                    oddlow   ( odd_low ),
                    oddhigh  ( odd_high )
  { this->SetStartPosition(); }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &signalIn, 
                  OutputPortalType            &coeffOut,
                  const vtkm::Id &workIndex) const
  {
    typedef typename OutputPortalType::ValueType OutputValueType;
    if( workIndex < approxLen + detailLen )
    {
      if( workIndex % 2 == 0 )  // calculate cA
      {
        vtkm::Id xl = xlstart + workIndex;
        VAL sum=MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
          sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get(xl++) );
        vtkm::Id outputIdx = workIndex / 2; // put cA at the beginning 
        coeffOut.Set( outputIdx, static_cast<OutputValueType>(sum) );
      }
      else                      // calculate cD
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xh = xhstart + workIndex - 1;
        for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
          sum += highFilter.Get(k) * MAKEVAL( signalIn.Get(xh++) );
        vtkm::Id outputIdx = approxLen + (workIndex-1) / 2; // put cD after cA
        coeffOut.Set( outputIdx, static_cast<OutputValueType>(sum) );
      }
    }
  }

  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen, approxLen, detailLen;  // filter and outcome coeff length.
  bool oddlow, oddhigh;
  vtkm::Id xlstart, xhstart;
  
  VTKM_EXEC_CONT_EXPORT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = this->oddhigh ? 1 : 0;
  }
};


// Worklet: perform a simple 2D inverse transform on odd length filters
template< typename DeviceTag >
class InverseTransform2DOdd: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayIn< ScalarAll >, // input extended signal
                                 FieldOut<     ScalarAll> ); // outptu coeffs
  typedef void ExecutionSignature( _1, _2, WorkIndex );                                    
  typedef _2   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransform2DOdd( const vtkm::cont::ArrayHandle<vtkm::Float64> &lo_fil,
                         const vtkm::cont::ArrayHandle<vtkm::Float64> &hi_fil,
                         vtkm::Id fil_len, vtkm::Id x1, vtkm::Id y1, vtkm::Id x2,
                         vtkm::Id y2, vtkm::Id cA_len_ext )
                       : lowFilter(  lo_fil.PrepareForInput( DeviceTag() ) ),
                         highFilter( hi_fil.PrepareForInput( DeviceTag() ) ),
                         filterLen( fil_len ), inputDimX( x1 ), inputDimY( y1 ),
                         outputDimX( x2 ), outputDimY( y2 ), cALenExtended( cA_len_ext ) {}

  VTKM_EXEC_CONT_EXPORT
  void Output1Dto2D( const vtkm::Id &idx, vtkm::Id &x, vtkm::Id &y ) const
  {
    x = idx % outputDimX;
    y = idx / outputDimX;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Input2Dto1D( vtkm::Id x, vtkm::Id y ) const     
  {
    return y * inputDimX + x; 
  }
  
  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template< typename InputPortalType, typename OutputValueType >
  VTKM_EXEC_EXPORT
  void operator() (const InputPortalType    &sigIn,
                         OutputValueType    &coeffOut,
                   const vtkm::Id           &workIdx ) const
  {
    vtkm::Id outX, outY;
    Output1Dto2D( workIdx, outX, outY );
    vtkm::Id inX = outX;
    vtkm::Id inY = outY;

    vtkm::Id k1, k2;
    VAL sum = 0.0;
    if( inX % 2 != 0 )
    {
      k1 = filterLen - 2;
      k2 = filterLen - 1;
    }
    else
    {
      k1 = filterLen - 1;
      k2 = filterLen - 2;
    }

    vtkm::Id xi = (inX + 1) / 2;
    vtkm::Id sigIdx1D;
    while( k1 > -1 )
    {
      sigIdx1D = Input2Dto1D( xi, inY );
      sum += lowFilter.Get(k1) * MAKEVAL( sigIn.Get( sigIdx1D ) );
      xi++;
      k1 -= 2;
    }
    xi = inX / 2;
    while( k2 > -1 )
    {
      sigIdx1D = Input2Dto1D( xi + cALenExtended, inY );
      sum += highFilter.Get(k2) * MAKEVAL( sigIn.Get( sigIdx1D ) );
      xi++;   
      k2 -= 2;
    }
    coeffOut = static_cast< OutputValueType> (sum);
  }
  
  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;
  const vtkm::Id inputDimX, inputDimY, outputDimX, outputDimY;
  const vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD
};


// Worklet: perform an inverse transform for odd length, symmetric filters.
template< typename DeviceTag >
class InverseTransformOdd: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayOut<ScalarAll>);   // output
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransformOdd( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                       const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                       vtkm::Id filLen, vtkm::Id ca_len, vtkm::Id ext_len ) :
                       lowFilter(  loFilter.PrepareForInput(DeviceTag()) ),
                       highFilter( hiFilter.PrepareForInput(DeviceTag()) ),
                       filterLen( filLen ), cALen( ca_len ),
                       cALen2( ca_len * 2 ), cALenExtended( ext_len )  {}
                       
  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &coeffs,
                  OutputPortalType            &sigOut,
                  const vtkm::Id &workIndex) const
  {
    if( workIndex < cALen2 )   // valid calculation region
    {
      vtkm::Id xi;         // coeff indices
      vtkm::Id k1, k2;     // indices for low and high filter
      VAL sum = 0.0;    

      if( workIndex % 2 != 0 )
      {
        k1 = this->filterLen - 2;
        k2 = this->filterLen - 1;
      }
      else
      {
        k1 = this->filterLen - 1;
        k2 = this->filterLen - 2;
      }

      xi = (workIndex+1) / 2;
      while( k1 > -1 )  // k1 >= 0
      {
        sum += lowFilter.Get(k1) * MAKEVAL( coeffs.Get(xi++) );
        k1 -= 2;
      }

      xi = workIndex / 2;
      while( k2 > -1 )  // k2 >= 0
      {
        sum += highFilter.Get(k2) * MAKEVAL( coeffs.Get( this->cALenExtended + xi++ ) );
        k2 -= 2;
      }
    
      sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>( sum ) );
    }

  }

  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;       // filter length.
  const vtkm::Id cALen;           // Number of actual cAs 
  const vtkm::Id cALen2;          //  = cALen * 2
  const vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD
};


// Worklet: perform an inverse transform for even length, symmetric filters.
template< typename DeviceTag >
class InverseTransform2DEven: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                FieldOut<ScalarAll>);        // output
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _2   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransform2DEven( const vtkm::cont::ArrayHandle<vtkm::Float64> &lo_fil,
                          const vtkm::cont::ArrayHandle<vtkm::Float64> &hi_fil,
                          vtkm::Id filtL, vtkm::Id x1, vtkm::Id y1, 
                          vtkm::Id x2, vtkm::Id y2, vtkm::Id cALExt )
                        : lowFilter(  lo_fil.PrepareForInput( DeviceTag() ) ),
                          highFilter( hi_fil.PrepareForInput( DeviceTag() ) ),
                          filterLen(filtL), inputDimX( x1 ), inputDimY( y1 ),
                          outputDimX( x2 ), outputDimY( y2 ), cALenExtended(cALExt) {}

  VTKM_EXEC_CONT_EXPORT
  void Output1Dto2D( const vtkm::Id &idx, vtkm::Id &x, vtkm::Id &y ) const
  {
    x = idx % outputDimX;
    y = idx / outputDimX;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Input2Dto1D( vtkm::Id x, vtkm::Id y ) const     
  {
    return y * inputDimX + x; 
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType, typename OutputValueType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &coeffs,
                  OutputValueType             &sigOut,
                  const vtkm::Id              &workIndex) const
  {
    vtkm::Id outX, outY;
    Output1Dto2D( workIndex, outX, outY );
    vtkm::Id inX = outX;
    vtkm::Id inY = outY;
    vtkm::Id xi, k;
    VAL sum = 0.0;    

    if( (filterLen/2) % 2 != 0 )  // odd length half filter
    {
      xi = inX / 2;
      if( inX % 2 != 0 )
        k = filterLen - 1;
      else
        k = filterLen - 2;
    }
    else
    {
      xi = (inX + 1) / 2;
      if( inX % 2 != 0 )
        k = filterLen - 2;
      else
        k = filterLen - 1;
    }

    vtkm::Id cAIdx1D, cDIdx1D; 
    while( k > -1 )   // k >= 0
    {
      cAIdx1D = Input2Dto1D( xi, inY );
      cDIdx1D = Input2Dto1D( xi + cALenExtended, inY );
      sum += lowFilter.Get(k)  * MAKEVAL( coeffs.Get( cAIdx1D ) ) +   // cA
             highFilter.Get(k) * MAKEVAL( coeffs.Get( cDIdx1D ) );    // cD
      xi++;
      k -= 2;
    }
    sigOut = static_cast<OutputValueType>( sum );
  }

  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;       // filter length.
  const vtkm::Id inputDimX, inputDimY, outputDimX, outputDimY;
  const vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD 
};    


// Worklet: perform an inverse transform for even length, symmetric filters.
template< typename DeviceTag >
class InverseTransformEven: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayOut<ScalarAll>);   // output
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransformEven( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                        const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                        vtkm::Id filtL, vtkm::Id cAL, vtkm::Id cALExt, bool m ) : 
                        lowFilter(  loFilter.PrepareForInput(DeviceTag()) ),
                        highFilter( hiFilter.PrepareForInput(DeviceTag()) ),
                        filterLen(  filtL ), cALen( cAL ), cALen2( cAL * 2 ),
                        cALenExtended( cALExt ), matlab( m )    {}

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &coeffs,
                  OutputPortalType            &sigOut,
                  const vtkm::Id &workIndex) const
  {
    if( workIndex < cALen2 )   // valid calculation region
    {
      vtkm::Id xi;         // coeff indices
      vtkm::Id k;          // indices for low and high filter
      VAL sum = 0.0;    

      if( matlab || (filterLen/2) % 2 != 0 )  // odd length half filter
      {
        xi = workIndex / 2;
        if( workIndex % 2 != 0 )
          k = filterLen - 1;
        else
          k = filterLen - 2;
      }
      else
      {
        xi = (workIndex + 1) / 2;
        if( workIndex % 2 != 0 )
          k = filterLen - 2;
        else
          k = filterLen - 1;
      }

      while( k > -1 )   // k >= 0
      {
        sum += lowFilter.Get(k)  * MAKEVAL( coeffs.Get( xi ) ) +               // cA
               highFilter.Get(k) * MAKEVAL( coeffs.Get( xi + cALenExtended) ); // cD
        xi++;
        k -= 2;
      }

      sigOut.Set(workIndex, static_cast<typename OutputPortalType::ValueType>( sum ) );
    }
  }

  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen;       // filter length.
  const vtkm::Id cALen;           // Number of actual cAs 
  const vtkm::Id cALen2;          //  = cALen * 2
  const vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD 
  bool     matlab;          // followed the naming convention from VAPOR 
                            // It's always false for the 1st 4 filters.
};    


// Worklet:
class ThresholdWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldInOut <ScalarAll> );  // Thresholding in-place
  typedef void  ExecutionSignature( _1 );
  typedef _1    InputDomain;

  // Constructor
  ThresholdWorklet( vtkm::Float64 t ) : threshold( t ),     // must pass in a positive val
                                        neg_threshold( t*-1.0 )  {}
  
  template <typename ValueType >
  VTKM_EXEC_EXPORT
  void operator()( ValueType    &coeffVal ) const
  {
    if( neg_threshold < coeffVal && coeffVal < threshold )
      coeffVal = 0.0;
  }

private:
  vtkm::Float64 threshold;      // positive 
  vtkm::Float64 neg_threshold;  // negative 
};    


// Worklet:
class SquaredDeviation: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldOut<ScalarAll>);        
                                                             
  typedef _2   ExecutionSignature( _1 );
  typedef _1   InputDomain;


  // Constructor
  template <typename ValueType>
  VTKM_EXEC_CONT_EXPORT
  SquaredDeviation( ValueType t ) 
  {
    this->mean = static_cast<vtkm::Float64>(t);
  }

  template <typename ValueType>
  VTKM_EXEC_EXPORT
  ValueType operator()( const ValueType &num ) const
  {
    vtkm::Float64 num64 = static_cast<vtkm::Float64>( num );
    vtkm::Float64 diff = this->mean - num64;
    return static_cast<ValueType>( diff * diff );
  }

private:
  vtkm::Float64 mean;

};   


// Worklet:
class Differencer: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldIn<ScalarAll>,
                                FieldOut<ScalarAll>);        
                                                             
  typedef _3   ExecutionSignature( _1, _2 );
  typedef _1   InputDomain;


  template <typename ValueType1, typename ValueType2 >
  VTKM_EXEC_EXPORT
  ValueType1 operator()( const ValueType1 &v1, const ValueType2 &v2 ) const
  {
    return v1 - static_cast<ValueType1>(v2);
  }

};   


// Worklet:
class SquareWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn< ScalarAll>,     
                                FieldOut<ScalarAll>);        
                                                             
  typedef _2   ExecutionSignature( _1 );
  typedef _1   InputDomain;

  template <typename ValueType>
  VTKM_EXEC_EXPORT
  ValueType operator()( const ValueType &v ) const
  {
    return (v * v);
  }
};    


// Worklet:
class CopyWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayIn<  ScalarAll >,
                                 WholeArrayOut< ScalarAll > );
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  CopyWorklet( vtkm::Id idx ) 
  {
    this->startIdx = idx;
  }

  template< typename PortalInType, typename PortalOutType >
  VTKM_EXEC_EXPORT
  void operator()( const PortalInType     &portalIn,
                         PortalOutType    &portalOut,
                   const vtkm::Id         &workIndex) const
  {
    portalOut.Set( (startIdx + workIndex), portalIn.Get(workIndex) );
  }

private:
  vtkm::Id startIdx;
};


// Worklet for signal extension no. 1
class LeftSYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  LeftSYMHExtentionWorklet( vtkm::Id len ) : addLen( len ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->addLen - workIndex - 1) );
  }

private:
  vtkm::Id addLen;
};


// Worklet for signal extension no. 2
class LeftSYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  LeftSYMWExtentionWorklet( vtkm::Id len ) : addLen( len ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->addLen - workIndex) );
  }

private:
  vtkm::Id addLen;
};


// Worklet for signal extension no. 3
class LeftASYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  LeftASYMHExtentionWorklet( vtkm::Id len ) : addLen (len) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( addLen - workIndex - 1) * (-1.0) );
  }

private:
  vtkm::Id addLen;
};


// Worklet for signal extension no. 4
class LeftASYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  LeftASYMWExtentionWorklet( vtkm::Id len ) : addLen (len) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( addLen - workIndex ) * (-1.0) );
  }

private:
  vtkm::Id addLen;
};


// Worklet for 2D signal extension on the right
class RightExtensionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;
  typedef vtkm::Id Id;

  // Constructor
  VTKM_EXEC_CONT_EXPORT 
  RightExtensionWorklet2D( bool padZero, Id x1, Id y1, Id x2, Id y2, DWTMode m) :
        sigPadZero( padZero), extDimX( x1 ), extDimY( y1 ), 
        sigRealDimX( x2 ), sigRealDimY( y2 ), mode(m)       {}

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  void Ext1Dto2D( const Id &idx, Id &x, Id &y ) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  Id Sig2Dto1D( Id x, Id y ) const
  {
    return y * sigRealDimX + x;
  }

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    Id extX, extY;
    Id sigX;
    Id sigDimX = sigRealDimX;
    if( sigPadZero )  // pretent signal is padded a zero at the end
      sigDimX++;
    typename PortalOutType::ValueType sym = 1.0;
    Ext1Dto2D( workIndex, extX, extY );
    if      ( mode == SYMH )
      sigX = sigDimX - extX - 1;
    else if ( mode == SYMW )
      sigX = sigDimX - extX - 2; 
    else if ( mode == ASYMH )
    {
      sigX = sigDimX - extX - 1;
      sym  = -1.0;
    }
    else    // mode == ASYMW
    {
      sigX = sigDimX - extX - 2;
      sym  = -1.0;
    }
    if( sigX == sigRealDimX )           // copy from the imaginary zero
      portalOut.Set( workIndex, 0.0 );
    else
      portalOut.Set( workIndex, portalIn.Get( Sig2Dto1D(sigX, extY) ) * sym );
  }

private:
  const bool sigPadZero;                    // if to pretend that signal has zero padded
  const vtkm::Id extDimX, extDimY;
  const vtkm::Id sigRealDimX, sigRealDimY;  // real dimX of signal
  DWTMode  mode;
};


// Worklet for signal extension no. 5
class RightSYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  RightSYMHExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->sigInLen - workIndex - 1) );
  }

private:
  vtkm::Id sigInLen;
};


// Worklet for signal extension no. 6
class RightSYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  RightSYMWExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get(this->sigInLen - workIndex - 2) );
  }

private:
  vtkm::Id sigInLen;
};


// Worklet for signal extension no. 7
class RightASYMHExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  RightASYMHExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( sigInLen - workIndex - 1) * (-1.0) );
  }

private:
  vtkm::Id sigInLen;
};


// Worklet for signal extension no. 8
class RightASYMWExtentionWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  RightASYMWExtentionWorklet ( vtkm::Id sigInl ) : sigInLen( sigInl ) {}

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_CONT_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    portalOut.Set( workIndex, portalIn.Get( sigInLen - workIndex - 2) * (-1.0) );
  }

private:
  vtkm::Id sigInLen;
};


// Worklet
class AssignZeroWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayInOut< ScalarAll > );
  typedef void ExecutionSignature( _1, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  AssignZeroWorklet( vtkm::Id idx ) : zeroIdx( idx )  { }

  template< typename PortalType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalType   &array,
                   const vtkm::Id     &workIdx ) const
  {
    if( workIdx == this->zeroIdx )
      array.Set( workIdx, static_cast<typename PortalType::ValueType>(0.0) );
  }

private:
  vtkm::Id zeroIdx;
};


// Worklet
class AssignZero2DColumnWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayInOut< ScalarAll > );
  typedef void ExecutionSignature( _1, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  AssignZero2DColumnWorklet( vtkm::Id x, vtkm::Id y, vtkm::Id idx ) 
        : dimX( x ), dimY( y ), zeroIdx( idx )  { }

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  void GetLogicalDim( const Id &idx, Id &x, Id &y ) const
  {
    x = idx % dimX;
    y = idx / dimX;
  }

  template< typename PortalType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalType   &array,
                   const vtkm::Id     &workIdx ) const
  {
    vtkm::Id x, y;
    GetLogicalDim( workIdx, x, y );
    if( x == zeroIdx )    // assign zero to a column
      array.Set( workIdx, static_cast<typename PortalType::ValueType>(0.0) );
  }

private:
  vtkm::Id dimX, dimY, zeroIdx;
};


// Worklet:
class TransposeWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldIn      < ScalarAll >,
                                 WholeArrayOut< ScalarAll > );
  typedef void ExecutionSignature( _1, _2, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  TransposeWorklet( vtkm::Id inx, vtkm::Id iny, vtkm::Id outx, vtkm::Id outy,
                    vtkm::Id out_startx, vtkm::Id out_starty )
                :   inXDim( inx ), inYDim( iny ), outXDim( outx ), outYDim( outy ),
                    outStartX( out_startx ), outStartY( out_starty )  {}

  VTKM_EXEC_CONT_EXPORT
  void Input1Dto2D( const vtkm::Id &idx, vtkm::Id &x, vtkm::Id &y ) const     
  {
    x = idx % inXDim;
    y = idx / inXDim;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Output2Dto1D( vtkm::Id &x, vtkm::Id &y ) const     
  {
    return y * outXDim + x;
  }

  template< typename ValueInType, typename PortalOutType >
  VTKM_EXEC_EXPORT
  void operator()( const ValueInType    &valueIn,
                         PortalOutType  &arrayOut,
                   const vtkm::Id       &workIdx ) const
  {
    vtkm::Id x, y;
    Input1Dto2D( workIdx, x, y );
    vtkm::Id outX = y + outStartX;
    vtkm::Id outY = x + outStartY;
    vtkm::Id outputIdx = Output2Dto1D( outX, outY );
    arrayOut.Set( outputIdx, valueIn );
  }

private:
  vtkm::Id inXDim,    inYDim;
  vtkm::Id outXDim,   outYDim;
  vtkm::Id outStartX, outStartY;
};


// Worklet:
// Copys a small rectangle to part of a big rectangle
// WARNING: this worklet only supports basic ArrayHandle types.
class RectangleCopyTo : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldIn<       ScalarAll >,    // Input, small rectangle
                                 WholeArrayOut< ScalarAll > );  // Output, big rectangle
  typedef void ExecutionSignature( _1, _2, WorkIndex );

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  RectangleCopyTo( vtkm::Id inx,       vtkm::Id iny, 
                   vtkm::Id outx,      vtkm::Id outy,
                   vtkm::Id xStart,    vtkm::Id yStart )
  {
    this->inXLen    = inx;      this->inYLen    = iny;
    this->outXLen   = outx;     this->outYLen   = outy;
    this->outXStart = xStart;   this->outYStart = yStart;
  }

  VTKM_EXEC_CONT_EXPORT
  void GetLogicalDimOfInputRect( const vtkm::Id    &idx,    
                                       vtkm::Id    &x,      
                                       vtkm::Id    &y ) const     
  {
    x = idx % inXLen;
    y = idx / inXLen;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Get1DIdxOfOutputRect( vtkm::Id    x,      
                                 vtkm::Id    y ) const     
  {
    return y * outXLen + x;
  }

  template< typename ValueInType, typename PortalOutType >
  VTKM_EXEC_EXPORT
  void operator()( const ValueInType    &valueIn,
                         PortalOutType  &arrayOut,
                   const vtkm::Id       &workIdx ) const
  {
    vtkm::Id xOfIn, yOfIn;
    GetLogicalDimOfInputRect( workIdx, xOfIn, yOfIn );
    vtkm::Id outputIdx = Get1DIdxOfOutputRect( xOfIn+outXStart, yOfIn+outYStart );
    arrayOut.Set( outputIdx, valueIn );
  }

private:
  vtkm::Id inXLen,    inYLen;
  vtkm::Id outXLen,   outYLen;
  vtkm::Id outXStart, outYStart;
};


// Worklet:
// Copys a part of a big rectangle to a small rectangle
// WARNING: this worklet only supports basic ArrayHandle types.
class RectangleCopyFrom : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( FieldInOut<   ScalarAll >,    // small rectangle to be filled
                                 WholeArrayIn< ScalarAll > );  // big rectangle to read from
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  RectangleCopyFrom( vtkm::Id smallx,    vtkm::Id smally, 
                     vtkm::Id bigx,      vtkm::Id bigy,
                     vtkm::Id xStart,    vtkm::Id yStart )
  {
    this->smallXLen = smallx;   this->smallYLen = smally;
    this->bigXLen   = bigx;     this->bigYLen   = bigy;
    this->bigXStart = xStart;   this->bigYStart = yStart;
  }

  VTKM_EXEC_CONT_EXPORT
  void GetLogicalDimOfSmallRect( const vtkm::Id    &idx,    
                                       vtkm::Id    &x,      
                                       vtkm::Id    &y ) const     
  {
    x = idx % smallXLen;
    y = idx / smallXLen;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Get1DIdxOfBigRect( vtkm::Id    x,      
                              vtkm::Id    y ) const     
  {
    return y * bigXLen + x;
  }

  template< typename ValueType, typename PortalType >
  VTKM_EXEC_EXPORT
  void operator()(       ValueType      &value,        
                   const PortalType     &array,
                   const vtkm::Id       &workIdx ) const
  {
    vtkm::Id xOfValue, yOfValue;
    GetLogicalDimOfSmallRect( workIdx, xOfValue, yOfValue );
    vtkm::Id bigRectIdx = Get1DIdxOfBigRect( xOfValue+bigXStart, yOfValue+bigYStart );
    value = static_cast<ValueType>( array.Get( bigRectIdx ) );
  }

private:
  vtkm::Id smallXLen,    smallYLen;
  vtkm::Id bigXLen,      bigYLen;
  vtkm::Id bigXStart,    bigYStart;
};

/*
 * put old implementations below this line
 */

class LeftExtensionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;
  typedef vtkm::Id Id;

  // Constructor
  VTKM_EXEC_CONT_EXPORT 
  LeftExtensionWorklet2D( Id x1, Id y1, Id x2, Id y2, DWTMode m)
      : extDimX( x1 ), extDimY( y1 ), sigDimX( x2 ), sigDimY( y2 ), mode(m)  {}

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  void Ext1Dto2D ( const Id &idx, Id &x, Id &y ) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  Id Sig2Dto1D( Id x, Id y ) const
  {
    return y * sigDimX + x;
  }

  template< typename PortalOutType, typename PortalInType >
  VTKM_EXEC_EXPORT
  void operator()(       PortalOutType       &portalOut,
                   const PortalInType        &portalIn,
                   const vtkm::Id            &workIndex) const
  {
    Id extX, extY;
    Id sigX;
    typename PortalOutType::ValueType sym = 1.0;
    Ext1Dto2D( workIndex, extX, extY );
    if      ( mode == SYMH )
      sigX = extDimX - extX - 1;
    else if ( mode == SYMW )
      sigX = extDimX - extX; 
    else if ( mode == ASYMH )
    {
      sigX = extDimX - extX - 1;
      sym  = -1.0;
    }
    else    // mode == ASYMW
    {
      sigX = extDimX - extX;
      sym  = -1.0;
    }
    portalOut.Set( workIndex, portalIn.Get( Sig2Dto1D(sigX, extY) ) * sym );
  }

private:
  vtkm::Id extDimX, extDimY, sigDimX, sigDimY;
  DWTMode  mode;
};


// Worklet: perform a simple 2D forward transform
template< typename DeviceTag >
class ForwardTransform2D: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, WorkIndex);
  typedef _2   InputDomain;


  // Constructor
  VTKM_EXEC_CONT_EXPORT
  ForwardTransform2D( const vtkm::cont::ArrayHandle<vtkm::Float64> &loFilter,
                      const vtkm::cont::ArrayHandle<vtkm::Float64> &hiFilter,
                      vtkm::Id filter_len, vtkm::Id approx_len, 
                      vtkm::Id input_dimx, vtkm::Id input_dimy,
                      vtkm::Id output_dimx, vtkm::Id output_dimy, bool odd_low )
                   :  lowFilter(  loFilter.PrepareForInput( DeviceTag() ) ),
                      highFilter( hiFilter.PrepareForInput( DeviceTag() ) ),
                      filterLen(  filter_len ), approxLen(  approx_len ),
                      inputDimX(  input_dimx ), inputDimY(  input_dimy ),
                      outputDimX( output_dimx), outputDimY( output_dimy),
                      oddlow( odd_low )
  { this->SetStartPosition(); }

  VTKM_EXEC_CONT_EXPORT
  void Input1Dto2D( const vtkm::Id &idx, vtkm::Id &x, vtkm::Id &y ) const     
  {
    x = idx % inputDimX;
    y = idx / inputDimX;
  }
  VTKM_EXEC_CONT_EXPORT
  void Output1Dto2D( const vtkm::Id &idx, vtkm::Id &x, vtkm::Id &y ) const     
  {
    x = idx % outputDimX;
    y = idx / outputDimX;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Input2Dto1D( const vtkm::Id &x, const vtkm::Id &y ) const     
  {
    return y * inputDimX + x;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Output2Dto1D( const vtkm::Id &x, const vtkm::Id &y ) const     
  {
    return y * outputDimX + x;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType, typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &signalIn, 
                  OutputPortalType            &coeffOut,
                  const vtkm::Id              &workIndex) const
  {
    vtkm::Id outputX, outputY;
    Output1Dto2D( workIndex, outputX, outputY );
    vtkm::Id inputX = outputX; 
    vtkm::Id inputY = outputY;
    
    vtkm::Id idx1D;
    typedef typename OutputPortalType::ValueType OutputValueType;
    if( inputX % 2 == 0 )  // calculate cA
    {
      vtkm::Id xl = xlstart + inputX;
      VAL sum=MAKEVAL(0.0);
      for( vtkm::Id k = filterLen - 1; k > -1; k-- )
      {
        idx1D = Input2Dto1D( xl, inputY );
        sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get( idx1D ) );
        xl++;
      }
      vtkm::Id dstX = inputX / 2; // put cA at the beginning 
      idx1D = Output2Dto1D( dstX, outputY );
      coeffOut.Set( idx1D, static_cast<OutputValueType>(sum) );
    }
    else                      // calculate cD
    {
      vtkm::Id xh = xhstart + inputX - 1;
      VAL sum=MAKEVAL(0.0);
      for( vtkm::Id k = filterLen - 1; k > -1; k-- )
      {
        idx1D = Input2Dto1D( xh, inputY );
        sum += highFilter.Get(k) * MAKEVAL( signalIn.Get( idx1D ) );
        xh++;
      }
      vtkm::Id dstX = approxLen + (inputX-1) / 2; // put cD after cA
      idx1D = Output2Dto1D( dstX, outputY );
      coeffOut.Set( idx1D, static_cast<OutputValueType>(sum) );
    }
  }

  #undef MAKEVAL
  #undef VAL

private:
  const typename vtkm::cont::ArrayHandle<vtkm::Float64>::ExecutionTypes<DeviceTag>::
      PortalConst lowFilter, highFilter;
  const vtkm::Id filterLen, approxLen;
  const vtkm::Id inputDimX, inputDimY, outputDimX, outputDimY;
  bool oddlow;
  vtkm::Id xlstart, xhstart;
  
  VTKM_EXEC_CONT_EXPORT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = 1;
  }
};

}     // namespace wavelets
}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
