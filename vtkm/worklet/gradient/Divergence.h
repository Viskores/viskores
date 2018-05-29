//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
  using ControlSignature = void(FieldIn<DivergenceTypes> input, FieldOut<Scalar> output);
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
