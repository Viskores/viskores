//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_gradient_Divergence_h
#define vtk_m_worklet_gradient_Divergence_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace gradient
{

struct DivergenceTypes : vtkm::ListTagBase<vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 3>,
                                           vtkm::Vec<vtkm::Vec<vtkm::Float64, 3>, 3>>
{
};


struct Divergence : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn input, FieldOut output);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  template <typename InputType, typename OutputType>
  VTKM_EXEC void operator()(const InputType& input, OutputType& divergence) const
  {
    divergence = input[0][0] + input[1][1] + input[2][2];
  }
};
}
}
}
#endif
