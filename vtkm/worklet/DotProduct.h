//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DotProduct_h
#define vtk_m_worklet_DotProduct_h

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VectorAnalysis.h>

namespace vtkm
{
namespace worklet
{

class DotProduct : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T, vtkm::IdComponent Size>
  VTKM_EXEC void operator()(const vtkm::Vec<T, Size>& v1,
                            const vtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = vtkm::Dot(v1, v2);
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Normalize_h
