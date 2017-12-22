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
#ifndef vtk_m_worklet_CrossProduct_h
#define vtk_m_worklet_CrossProduct_h

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{

class CrossProduct : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<VecAll>, FieldIn<VecAll>, FieldOut<VecAll>);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename T, typename T2>
  VTKM_EXEC void operator()(const T& vec1, const T& vec2, T2& outVec) const
  {
    outVec = vtkm::Cross(vec1, vec2);
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_CrossProduct_h
