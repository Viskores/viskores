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

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {

namespace internal{

  const vtkm::Float64 hm4_44[9] = {
    0.037828455507264,
    -0.023849465019557,
    -0.110624404418437,
    0.377402855612831,
    0.852698679008894,
    0.377402855612831,
    -0.110624404418437,
    -0.023849465019557,
    0.037828455507264
  };

  const vtkm::Float64 h4[9] = {
    0.0,
    -0.064538882628697,
    -0.040689417609164,
    0.418092273221617,
    0.788485616405583,
    0.418092273221617,
    -0.0406894176091641,
    -0.0645388826286971,
    0.0
  };
}

class Wavelets
{
public:

  // helper worklet
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

    // ForwardTransform constructor
    VTKM_CONT_EXPORT
    ForwardTransform() 
    {
      magicNum  = 3.14159265;
      filterLen = 9;
      oddlow    = oddhigh = false;
      xlstart   = xhstart = 0;
    }

    // Specify odd or even for low and high coeffs
    VTKM_CONT_EXPORT
    void SetOddness(const bool &odd_low, const bool &odd_high )
    {
      this->oddlow  = odd_low;
      this->xlstart = odd_low ? 1 : 0;

      this->oddhigh = odd_high;
      this->xhstart = odd_high ? 1 : 0;
    }

    // Set the filter length
    VTKM_CONT_EXPORT
    void SetFilterLength(const vtkm::Id &len )
    {
      this->filterLen = len;
    }

    // Use 64-bit float for internal calculation
    #define VAL        vtkm::Float64
    #define MAKEVAL(a) (static_cast<VAL>(a))

    template <typename InputSignalPortalType,
              typename FilterPortalType,
              typename OutputCoeffType>
    VTKM_EXEC_EXPORT
    void operator()(const InputSignalPortalType &signalIn, 
                    const FilterPortalType      &lowFilter,
                    const FilterPortalType      &highFilter,
                    OutputCoeffType &coeffOut,
                    const vtkm::Id &workIndex) const
    {
      if( workIndex % 2 == 0 )    // calculate cA, approximate coeffs
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xl = xlstart + workIndex;
        if( xl + filterLen < signalIn.GetNumberOfValues() )
        {
          for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
            sum += lowFilter.Get(k) * MAKEVAL( signalIn.Get(xl++) );
          coeffOut = static_cast<OutputCoeffType>( sum );
        }
        else
          coeffOut = static_cast<OutputCoeffType>( magicNum );
      }
      else                        // calculate cD, detail coeffs
      {
        VAL sum=MAKEVAL(0.0);
        vtkm::Id xh = xhstart + workIndex - 1;
        if( xh + filterLen < signalIn.GetNumberOfValues() )
        {
          for( vtkm::Id k = filterLen - 1; k >= 0; k-- )
            sum += highFilter.Get(k) * MAKEVAL( signalIn.Get(xh++) );
          coeffOut = static_cast<OutputCoeffType>( sum );
        }
        else
          coeffOut = static_cast<OutputCoeffType>( magicNum );
      }
    }

    #undef MAKEVAL
    #undef VAL

  private:
    vtkm::Float64 magicNum;
    vtkm::Id      filterLen, xlstart, xhstart;
    bool oddlow, oddhigh;


  };  // class ForwardTransform

};    // class Wavelets

}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
