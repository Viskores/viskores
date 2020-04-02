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

using VorticityTypes = vtkm::List<vtkm::Vec<vtkm::Vec3f_32, 3>, vtkm::Vec<vtkm::Vec3f_64, 3>>;


struct Vorticity : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn input, FieldOut output);

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
