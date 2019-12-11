//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_gradient_Transpose_h
#define vtk_m_worklet_gradient_Transpose_h

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
using TransposeType = vtkm::List<vtkm::Vec<vtkm::Vec<T, 3>, 3>>;

template <typename T>
struct Transpose3x3 : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut field);

  template <typename FieldInVecType>
  VTKM_EXEC void operator()(FieldInVecType& field) const
  {
    T tempA, tempB, tempC;
    tempA = field[0][1];
    field[0][1] = field[1][0];
    field[1][0] = tempA;
    tempB = field[0][2];
    field[0][2] = field[2][0];
    field[2][0] = tempB;
    tempC = field[1][2];
    field[1][2] = field[2][1];
    field[2][1] = tempC;
  }

  template <typename S>
  void Run(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<T, 3>, 3>, S>& field,
           vtkm::cont::DeviceAdapterId device = vtkm::cont::DeviceAdapterTagAny())
  {
    vtkm::worklet::DispatcherMapField<Transpose3x3<T>> dispatcher;
    dispatcher.SetDevice(device);
    dispatcher.Invoke(field);
  }
};
}
}
}

#endif
