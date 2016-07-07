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
                                  WholeArrayIn<vtkm::Float64>, // lowFilter
                                  WholeArrayIn<vtkm::Float64>, // highFilter
                                  FieldOut<ScalarAll>);        // cA in even indices, 
                                                               // cD in odd indices
    typedef void ExecutionSignature(_1, _2, _3, _4, WorkIndex);
    typedef _1   InputDomain;

//    typedef vtkm::Float64 FLOAT;

    // ForwardTransform constructor
    VTKM_CONT_EXPORT
    ForwardTransform() 
    {
      magicNum = 2.0;
      oddlow   = true;
      oddhigh  = true;
    }


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
        vtkm::Float64 tmp  = static_cast<vtkm::Float64>(signalIn.Get(workIndex));
        if( workIndex % 2 == 0 )    // calculate cA, approximate coeffs
        {
          coeffOut = static_cast<OutputCoeffType>( tmp + lowFilter.Get(0) );
        }
        else                        // calculate cD, detail coeffs
        {
          coeffOut = static_cast<OutputCoeffType>( tmp + highFilter.Get(0) );
        }
    }

  private:
    vtkm::Float64 magicNum;
    bool oddlow, oddhigh;
  };  // class ForwardTransform

};    // class Wavelets

}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
