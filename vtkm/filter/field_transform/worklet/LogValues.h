//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_LogValues_h
#define vtk_m_worklet_LogValues_h

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{
template <vtkm::FloatDefault LogFunc(vtkm::FloatDefault)>
class LogFunWorklet : public vtkm::worklet::WorkletMapField
{
  const vtkm::FloatDefault MinValue;

public:
  VTKM_CONT
  LogFunWorklet(const vtkm::FloatDefault minValue)
    : MinValue(minValue)
  {
  }

  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const T& value, vtkm::FloatDefault& log_value) const
  {
    vtkm::FloatDefault f_value = static_cast<vtkm::FloatDefault>(value);
    f_value = vtkm::Max(MinValue, f_value);
    log_value = LogFunc(f_value);
  }
}; //class LogFunWorklet
}
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_LogValues_h
