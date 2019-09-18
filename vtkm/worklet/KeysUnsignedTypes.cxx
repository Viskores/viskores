//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define vtk_m_worklet_Keys_cxx
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/Keys.hxx>

#define VTK_M_KEYS_EXPORT(T)                                                                       \
  template class VTKM_WORKLET_EXPORT vtkm::worklet::Keys<T>;                                       \
  template VTKM_WORKLET_EXPORT VTKM_CONT void vtkm::worklet::Keys<T>::BuildArrays(                 \
    const vtkm::cont::ArrayHandle<T>& keys,                                                        \
    vtkm::worklet::KeysSortType sort,                                                              \
    vtkm::cont::DeviceAdapterId device);                                                           \
  template VTKM_WORKLET_EXPORT VTKM_CONT void vtkm::worklet::Keys<T>::BuildArrays(                 \
    const vtkm::cont::ArrayHandleVirtual<T>& keys,                                                 \
    vtkm::worklet::KeysSortType sort,                                                              \
    vtkm::cont::DeviceAdapterId device)

VTK_M_KEYS_EXPORT(vtkm::HashType);
VTK_M_KEYS_EXPORT(vtkm::UInt8);
using Pair_UInt8_Id2 = vtkm::Pair<vtkm::UInt8, vtkm::Id2>;
VTK_M_KEYS_EXPORT(Pair_UInt8_Id2);


#undef VTK_M_KEYS_EXPORT
