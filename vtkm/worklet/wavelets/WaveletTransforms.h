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
    //else
      //coeffOut.Set( workIndex, static_cast<OutputValueType>( magicNum ) );
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

#if 0
// Worklet: perform an inverse transform for odd length, symmetric filters.
class InverseTransformOdd: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs,
                                                             // cA followed by cD
                                WholeArrayIn<Scalar>,        // lowFilter
                                WholeArrayIn<Scalar>,        // highFilter
                                FieldOut<ScalarAll>);        // output
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_EXEC_CONT_EXPORT
  InverseTransformOdd() 
  {
    magicNum  = 0.0;
    filterLen = 0;
    cALen = 0;
  }

  // Set the filter length
  VTKM_EXEC_CONT_EXPORT
  void SetFilterLength( vtkm::Id len )
  {
    VTKM_ASSERT( len % 2 == 1 );
    this->filterLen = len;
  }

  // Set cA length
  VTKM_EXEC_CONT_EXPORT
  void SetCALength( vtkm::Id len, vtkm::Id lenExt )
  {
    this->cALen = len;
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
    vtkm::Id xi;    // coeff indices
    vtkm::Id k;     // filter indices

    VAL sum = 0.0;    

    if( workIndex < 2*cALen )   // valid calculation region
    {
      xi = (workIndex+1) / 2;
      if( workIndex % 2 != 0 )
        k = this->filterLen - 2;
      else
        k = this->filterLen - 1;
      while( k >= 0 )
      {
        sum += lowFilter.Get(k) * MAKEVAL( coeffs.Get(xi) );
        xi++;
        k -= 2;
      }

      xi = workIndex / 2;
      if( workIndex % 2 != 0 )
        k = this->filterLen - 1;
      else
        k = this->filterLen - 2;
      while( k >= 0 )
      {
        sum += highFilter.Get(k) * MAKEVAL( coeffs.Get( xi + this->cALenExtended ) );
        xi++;
        k -= 2;
      }
    }

    sigOut = static_cast<OutputPortalType>( sum );
  }

  #undef MAKEVAL
  #undef VAL

private:
  vtkm::Float64 magicNum;
  vtkm::Id filterLen;       // filter length.
  vtkm::Id cALen;           // Number of actual cAs 
  vtkm::Id cALenExtended;   // Number of extended cA at the beginning of input array
  
};    // class ForwardTransform
#endif


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
  InverseTransformOdd() 
  {
    magicNum  = 0.0;
    filterLen = 0;
    cALen = 0;
  }

  // Set the filter length
  VTKM_EXEC_CONT_EXPORT
  void SetFilterLength( vtkm::Id len )
  {
    VTKM_ASSERT( len % 2 == 1 );
    this->filterLen = len;
  }

  // Set cA length
  VTKM_EXEC_CONT_EXPORT
  void SetCALength( vtkm::Id len, vtkm::Id lenExt )
  {
    this->cALen = len;
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
    vtkm::Id xi;    // coeff indices
    vtkm::Id k;     // filter indices

    if( workIndex < 2*cALen )   // valid calculation region
    {
      VAL sum = 0.0;    

      xi = (workIndex+1) / 2;
      if( workIndex % 2 != 0 )
        k = this->filterLen - 2;
      else
        k = this->filterLen - 1;
      while( k >= 0 )
      {
        sum += lowFilter.Get(k) * MAKEVAL( coeffs.Get(xi) );
        xi++;
        k -= 2;
      }

      xi = workIndex / 2;
      if( workIndex % 2 != 0 )
        k = this->filterLen - 1;
      else
        k = this->filterLen - 2;
      while( k >= 0 )
      {
        sum += highFilter.Get(k) * MAKEVAL( coeffs.Get( xi + this->cALenExtended ) );
        xi++;
        k -= 2;
      }
    
      sigOut.Set(workIndex, 
                 static_cast<typename OutputPortalType::ValueType>( sum ) );
    }

  }

  #undef MAKEVAL
  #undef VAL

private:
  vtkm::Float64 magicNum;
  vtkm::Id filterLen;       // filter length.
  vtkm::Id cALen;           // Number of actual cAs 
  vtkm::Id cALenExtended;   // Number of extended cA at the beginning of input array
  
};    // class ForwardTransform


class ThresholdWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldOut<ScalarAll>);        
                                                             
  typedef _2   ExecutionSignature( _1 );
  typedef _1   InputDomain;


  // Constructor
  template <typename ValueType>
  VTKM_EXEC_CONT_EXPORT
  ThresholdWorklet( ValueType t ) 
  {
    this->threshold = static_cast<vtkm::Float64>(t);
    this->negThreshold = threshold * -1.0;
  }

  template <typename ValueType>
  VTKM_EXEC_EXPORT
  ValueType operator()( const ValueType &coeff ) const
  {
    vtkm::Float64 coeff64 = static_cast<vtkm::Float64>( coeff );
    if( coeff64 > negThreshold && coeff64 < threshold )
      return 0.0;
    else
      return coeff;
  }

private:
  vtkm::Float64 threshold;
  vtkm::Float64 negThreshold;

};    


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


class Differencer: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldIn<ScalarAll>,
                                FieldOut<ScalarAll>);        
                                                             
  typedef _3   ExecutionSignature( _1, _2 );
  typedef _1   InputDomain;


  template <typename ValueType>
  VTKM_EXEC_EXPORT
  ValueType operator()( const ValueType &v1, const ValueType &v2 ) const
  {
    return (v1 - v2);
  }

};   


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
  VTKM_EXEC_CONT_EXPORT
  void operator()( const PortalInType     &portalIn,
                         PortalOutType    &portalOut,
                   const vtkm::Id         &workIndex) const
  {
    portalOut.Set( (startIdx + workIndex), portalIn.Get(workIndex) );
  }

private:
  vtkm::Id startIdx;
};


}     // namespace wavelets
}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
