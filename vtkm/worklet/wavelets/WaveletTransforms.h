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
                                FieldOut<ScalarAll>);        // cA in even indices, 
                                                             // cD in odd indices
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _1   InputDomain;


  // Constructor
  VTKM_CONT_EXPORT
  ForwardTransform() 
  {
    magicNum  = 0.0;
    oddlow    = oddhigh   = true;
    filterLen = approxLen = detailLen = 0;
    this->SetStartPosition();
  }

  // Specify odd or even for low and high coeffs
  VTKM_CONT_EXPORT
  void SetOddness(bool odd_low, bool odd_high )
  {
    this->oddlow  = odd_low;
    this->oddhigh = odd_high;
    this->SetStartPosition();
  }

  // Set the filter length
  VTKM_CONT_EXPORT
  void SetFilterLength( vtkm::Id len )
  {
    this->filterLen = len;
  }

  // Set the outcome coefficient length
  VTKM_CONT_EXPORT
  void SetCoeffLength( vtkm::Id approx_len, vtkm::Id detail_len )
  {
    this->approxLen = approx_len;
    this->detailLen = detail_len;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputSignalPortalType,
            typename FilterPortalType,
            typename OutputCoeffType>
  VTKM_EXEC_EXPORT
  void operator()(const InputSignalPortalType &signalIn, 
                  const FilterPortalType      &lowFilter,
                  const FilterPortalType      &highFilter,
                  OutputCoeffType             &coeffOut,
                  const vtkm::Id &workIndex) const
  {
    if( workIndex % 2 == 0 )    // calculate cA, approximate coeffs
      if( workIndex < approxLen + detailLen )
      {
        vtkm::Id xl = xlstart + workIndex;
        VAL sum=MAKEVAL(0.0);
        for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
          sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get(xl++) );
        coeffOut = static_cast<OutputCoeffType>( sum );
      }
      else
        coeffOut = static_cast<OutputCoeffType>( magicNum );
    else                        // calculate cD, detail coeffs
      if( workIndex < approxLen + detailLen )
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xh = xhstart + workIndex - 1;
        for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
          sum += highFilter.Get(k) * MAKEVAL( signalIn.Get(xh++) );
        coeffOut = static_cast<OutputCoeffType>( sum );
      }
      else
        coeffOut = static_cast<OutputCoeffType>( magicNum );
  }

  #undef MAKEVAL
  #undef VAL

private:
  vtkm::Float64 magicNum;
  vtkm::Id filterLen, approxLen, detailLen;  // filter and outcome coeff length.
  vtkm::Id xlstart, xhstart;
  bool oddlow, oddhigh;
  
  VTKM_CONT_EXPORT
  void SetStartPosition()
  {
    this->xlstart = this->oddlow  ? 1 : 0;
    this->xhstart = this->oddhigh ? 1 : 0;
  }
};    // Finish class ForwardTransform


// Worklet: perform an inverse transform for odd length, symmetric filters.
class InverseTransformOdd: public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn<ScalarAll>,     // Input: coeffs 
                                                             //        cA followed by cD
                                WholeArrayIn<Scalar>,        // lowFilter
                                WholeArrayIn<Scalar>,        // highFilter
                                FieldOut<ScalarAll>);        // output
  typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
  typedef _1   InputDomain;

  // Constructor
  VTKM_CONT_EXPORT
  InverseTransformOdd() 
  {
    magicNum  = 0.0;
    filterLen = 0;
    cALen = 0;
  }

  // Set the filter length
  VTKM_CONT_EXPORT
  void SetFilterLength( vtkm::Id len )
  {
    //VTKM_ASSERT( len % 2 == 1 );
    this->filterLen = len;
  }

  // Set cA length
  VTKM_CONT_EXPORT
  void SetCALength( vtkm::Id len, vtkm::Id lenExt )
  {
    this->cALen = len;
    this->cALenExtended = lenExt;
  }

  // Use 64-bit float for convolution calculation
  #define VAL        vtkm::Float64
  #define MAKEVAL(a) (static_cast<VAL>(a))

  template <typename InputCoeffPortalType,
            typename FilterPortalType,
            typename OutputSignalType>
  VTKM_EXEC_EXPORT
  void operator()(const InputCoeffPortalType  &coeffs,
                  const FilterPortalType      &lowFilter,
                  const FilterPortalType      &highFilter,
                  OutputSignalType            &sigOut,
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

    sigOut = static_cast<OutputSignalType>( sum );
  }

  #undef MAKEVAL
  #undef VAL

private:
  vtkm::Float64 magicNum;
  vtkm::Id filterLen;       // filter length.
  vtkm::Id cALen;           // Number of actual cAs 
  vtkm::Id cALenExtended;   // Number of extended cA at the beginning of input array
  
};    // class ForwardTransform


template <typename ValueType>
class ThresholdWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<ScalarAll>,     
                                FieldOut<ScalarAll>);        
                                                             
  typedef _2 ExecutionSignature( _1 );
  typedef _1   InputDomain;


  // Constructor
  VTKM_CONT_EXPORT
  ThresholdWorklet( ValueType t ) 
  {
    this->threshold = t;
    this->negThreshold = t * -1.0;
  }

  VTKM_EXEC_EXPORT
  ValueType operator()( const ValueType &coeff ) const
  {
    if( coeff > negThreshold && coeff < threshold )
      return 0.0;
    else
      return coeff;
  }

private:
  ValueType threshold;
  ValueType negThreshold;

};    // Finish ThresholdWorklet class


}     // namespace wavelets
}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
