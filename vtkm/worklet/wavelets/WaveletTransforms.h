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


// Worklet: perform a simple forward transform
class ForwardTransform: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayIn<Scalar>,        // lowFilter
                                WholeArrayIn<Scalar>,        // highFilter
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _1   InputDomain;


  // Constructor
  VTKM_EXEC_CONT_EXPORT
  ForwardTransform() 
  {
    magicNum  = 0.0;
    oddlow    = oddhigh   = true;
    filterLen = approxLen = detailLen = 0;
    this->SetStartPosition();
  }

  // Specify odd or even for low and high coeffs
  VTKM_EXEC_CONT_EXPORT
  void SetOddness(bool odd_low, bool odd_high )
  {
    this->oddlow  = odd_low;
    this->oddhigh = odd_high;
    this->SetStartPosition();
  }

  // Set the filter length
  VTKM_EXEC_CONT_EXPORT
  void SetFilterLength( vtkm::Id len )
  {
    this->filterLen = len;
  }

  // Set the outcome coefficient length
  VTKM_EXEC_CONT_EXPORT
  void SetCoeffLength( vtkm::Id approx_len, vtkm::Id detail_len )
  {
    this->approxLen = approx_len;
    this->detailLen = detail_len;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &signalIn, 
                  const FilterPortalType      &lowFilter,
                  const FilterPortalType      &highFilter,
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
  vtkm::Float64 magicNum;
  vtkm::Id filterLen, approxLen, detailLen;  // filter and outcome coeff length.
  vtkm::Id xlstart, xhstart;
  bool oddlow, oddhigh;
  
  VTKM_EXEC_CONT_EXPORT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = this->oddhigh ? 1 : 0;
  }
};    // Finish class ForwardTransform


// Worklet: perform a simple 2D forward transform
class ForwardTransform2D: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // sigIn
                                WholeArrayIn<Scalar>,        // lowFilter
                                WholeArrayIn<Scalar>,        // highFilter
                                WholeArrayOut<ScalarAll>);   // cA followed by cD
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _4   InputDomain;


  // Constructor
  VTKM_EXEC_CONT_EXPORT
  ForwardTransform2D( vtkm::Id filter_len, vtkm::Id approx_len, bool odd_low,
                      vtkm::Id input_dimx, vtkm::Id input_dimy,
                      vtkm::Id output_dimx, vtkm::Id output_dimy )
  {
    magicNum  = 0.0;
    filterLen = filter_len;
    approxLen = approx_len;
    oddlow    = odd_low;
    inputDimX = input_dimx;
    inputDimY = input_dimy;
    outputDimX = output_dimx;
    outputDimY = output_dimy;
    this->SetStartPosition();
  }

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
  vtkm::Id Input2Dto1D( vtkm::Id &x, vtkm::Id &y ) const     
  {
    return y * inputDimX + x;
  }
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Output2Dto1D( vtkm::Id &x, vtkm::Id &y ) const     
  {
    return y * outputDimX + x;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &signalIn, 
                  const FilterPortalType      &lowFilter,
                  const FilterPortalType      &highFilter,
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
  vtkm::Float64 magicNum;
  vtkm::Id filterLen, approxLen;  // filter and outcome coeff length.
  bool oddlow;
  vtkm::Id xlstart, xhstart;
  vtkm::Id inputDimX,  inputDimY;
  vtkm::Id outputDimX, outputDimY;
  
  VTKM_EXEC_CONT_EXPORT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = 1;
  }
};    


// Worklet: perform an inverse transform for odd length, symmetric filters.
class InverseTransformOdd: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayIn<Scalar>,        // lowFilter
                                WholeArrayIn<Scalar>,        // highFilter
                                WholeArrayOut<ScalarAll>);   // output
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransformOdd() : filterLen(0), cALen(0), cALen2(0), cALenExtended(0) {}

  // Set the filter length
  VTKM_EXEC_CONT_EXPORT
  void SetFilterLength( vtkm::Id len )
  {
    this->filterLen = len;
  }

  // Set cA length
  VTKM_EXEC_CONT_EXPORT
  void SetCALength( vtkm::Id len, vtkm::Id lenExt )
  {
    this->cALen         = len;
    this->cALen2        = len * 2;
    this->cALenExtended = lenExt;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &coeffs,
                  const FilterPortalType      &lowFilter,
                  const FilterPortalType      &highFilter,
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
  vtkm::Id filterLen;       // filter length.
  vtkm::Id cALen;           // Number of actual cAs 
  vtkm::Id cALen2;          //  = cALen * 2
  vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD
  
};    // class InverseTransformOdd


// Worklet: perform an inverse transform for even length, symmetric filters.
class InverseTransformEven: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayIn<Scalar>,        // lowFilter
                                WholeArrayIn<Scalar>,        // highFilter
                                WholeArrayOut<ScalarAll>);   // output
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransformEven( vtkm::Id filtL, vtkm::Id cAL, vtkm::Id cALExt, bool m ) : 
                        filterLen(filtL), cALen(cAL), cALenExtended(cALExt), matlab(m)
  { 
    this->cALen2 = cALen * 2;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputPortalType,
            typename FilterPortalType,
            typename OutputPortalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputPortalType       &coeffs,
                  const FilterPortalType      &lowFilter,
                  const FilterPortalType      &highFilter,
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
  vtkm::Id filterLen;       // filter length.
  vtkm::Id cALen;           // Number of actual cAs 
  vtkm::Id cALen2;          //  = cALen * 2
  vtkm::Id cALenExtended;   // Number of cA at the beginning of input, followed by cD 
  bool     matlab;          // followed the naming convention from VAPOR
  
};    // class InverseTransformEven


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


// Worklet for 2D signal extension on the left
class LeftExtentionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;
  typedef vtkm::Id Id;

  // Constructor
  VTKM_EXEC_CONT_EXPORT 
  LeftExtentionWorklet2D( Id x1, Id y1, Id x2, Id y2, DWTMode m)
      : extDimX( x1 ), extDimY( y1 ), sigDimX( x2 ), sigDimY( y2 ), mode(m)  {}

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  void GetExtLogicalDim( const Id &idx, Id &x, Id &y ) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  Id GetSignal1DIndex( Id x, Id y ) const
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
    GetExtLogicalDim( workIndex, extX, extY );
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
    portalOut.Set( workIndex, portalIn.Get( GetSignal1DIndex(sigX, extY) ) * sym );
  }

private:
  vtkm::Id extDimX, extDimY, sigDimX, sigDimY;
  DWTMode  mode;
};


// Worklet for 2D signal extension on the right
class RightExtentionWorklet2D : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature( WholeArrayOut < ScalarAll >,   // extension part
                                 WholeArrayIn  < ScalarAll > ); // signal part
  typedef void ExecutionSignature( _1, _2, WorkIndex );
  typedef _1   InputDomain;
  typedef vtkm::Id Id;

  // Constructor
  VTKM_EXEC_CONT_EXPORT 
  RightExtentionWorklet2D( Id x1, Id y1, Id x2, Id y2, DWTMode m)
      : extDimX( x1 ), extDimY( y1 ), sigDimX( x2 ), sigDimY( y2 ), mode(m)  {}

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  void GetExtLogicalDim( const Id &idx, Id &x, Id &y ) const
  {
    x = idx % extDimX;
    y = idx / extDimX;
  }

  // Index translation helper
  VTKM_EXEC_CONT_EXPORT
  Id GetSignal1DIndex( Id x, Id y ) const
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
    GetExtLogicalDim( workIndex, extX, extY );
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
    portalOut.Set( workIndex, portalIn.Get( GetSignal1DIndex(sigX, extY) ) * sym );
  }

private:
  vtkm::Id extDimX, extDimY, sigDimX, sigDimY;
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
  TransposeWorklet( vtkm::Id x, vtkm::Id y )
  {
    this->inXLen  = x;
    this->inYLen  = y;
    this->outXLen = y;
    this->outYLen = x;
  }

  VTKM_EXEC_CONT_EXPORT
  void GetLogicalDimOfInputMatrix( const vtkm::Id    &idx,    
                                         vtkm::Id    &x,      
                                         vtkm::Id    &y ) const     
  {
    x = idx % inXLen;
    y = idx / inXLen;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id Get1DIdxOfOutputMatrix( vtkm::Id    &x,      
                                   vtkm::Id    &y ) const     
  {
    return y * outXLen + x;
  }

  template< typename ValueInType, typename PortalOutType >
  VTKM_EXEC_EXPORT
  void operator()( const ValueInType    &valueIn,
                         PortalOutType  &arrayOut,
                   const vtkm::Id       &workIdx ) const
  {
    vtkm::Id x, y;
    GetLogicalDimOfInputMatrix( workIdx, x, y );
    vtkm::Id outputIdx = Get1DIdxOfOutputMatrix( y, x );
    arrayOut.Set( outputIdx, valueIn );
  }

private:
  vtkm::Id inXLen,  inYLen;
  vtkm::Id outXLen, outYLen;
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

}     // namespace wavelets
}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
