//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_gradient_Vorticity_h
#define vtk_m_worklet_gradient_Vorticity_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace gradient
{

struct VorticityTypes : vtkm::ListTagBase<vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 3>,
                                          vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3>>
{
};


struct Vorticity : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn input, FieldOut output);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename InputType, typename OutputType>
  VTKM_EXEC void operator()(const InputType& input, OutputType& vorticity) const
  {
    vorticity[0] = input[1][2] - input[2][1];
    vorticity[1] = input[2][0] - input[0][2];
    vorticity[2] = input[0][1] - input[1][0];
  }
};
}
}
}

#endif
