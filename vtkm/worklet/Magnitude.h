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
#ifndef vtk_m_worklet_Magnitude_h
#define vtk_m_worklet_Magnitude_h

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{

class Magnitude : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<VecAll>, FieldOut<Scalar>);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename T2>
  VTKM_EXEC void operator()(const T& inValue, T2& outValue) const
  {
    outValue = static_cast<T2>(vtkm::Magnitude(inValue));
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Magnitude_h
