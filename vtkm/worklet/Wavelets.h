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

class Wavelets
{
public:
  // helper worklet
  class ForwardTransform: public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(WholeArrayIn<ScalarAll>,  // sigIn
                                  FieldOut<ScalarAll>,      // cA
                                  FieldOut<ScalarAll>);     // cD
    typedef void ExecutionSignature(_1, _2, _3, WorkIndex);
    typedef _1   InputDomain;

    // ForwardTransform constructor
    VTKM_CONT_EXPORT
    ForwardTransform() 
    {
      magicNum = 2.0;
      oddlow   = true;
      oddhigh  = true;
    }


    template <typename T, typename ArrayPortalType>
    VTKM_EXEC_EXPORT
    void operator()(const ArrayPortalType &signalIn, 
                    T &coeffApproximation,
                    T &coeffDetail,
                    const vtkm::Id &workIndex) const
    {
        vtkm::Float64 tmp  = static_cast<vtkm::Float64>(signalIn.Get(workIndex));
        coeffApproximation = static_cast<T>( tmp / 2.0 );
        coeffDetail        = static_cast<T>( tmp * 2.0 );
    }

  private:
    vtkm::Float64 magicNum;
    bool oddlow, oddhigh;
  };  // class ForwardTransform

};    // class Wavelets

}     // namespace worlet
}     // namespace vtkm

#endif // vtk_m_worklet_Wavelets_h
