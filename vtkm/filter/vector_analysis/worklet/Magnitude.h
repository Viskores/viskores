//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  using ControlSignature = void(FieldIn, FieldOut);

  template <typename T, typename T2>
  VTKM_EXEC void operator()(const T& inValue, T2& outValue) const
  {
    outValue = static_cast<T2>(vtkm::Magnitude(inValue));
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Magnitude_h
