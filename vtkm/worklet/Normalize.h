//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Normalize_h
#define vtk_m_worklet_Normalize_h

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{

class Normal : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);

  template <typename T, typename T2>
  VTKM_EXEC void operator()(const T& inValue, T2& outValue) const
  {
    outValue = vtkm::Normal(inValue);
  }
};

class Normalize : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut);

  template <typename T>
  VTKM_EXEC void operator()(T& value) const
  {
    vtkm::Normalize(value);
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Normalize_h
