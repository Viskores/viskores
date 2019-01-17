//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define vtk_m_worklet_Keys_cxx
#include <vtkm/worklet/Keys.h>

#define VTK_M_RM_PAREN(T) vtkm::cont::detail::GetTypeInParentheses<void T>::type

#define VTK_M_KEYS_EXPORT_TYPE(T)                                                                  \
  template class VTKM_WORKLET_EXPORT vtkm::worklet::Keys<VTK_M_RM_PAREN(T)>;                       \
  template VTKM_WORKLET_EXPORT VTKM_CONT void vtkm::worklet::Keys<VTK_M_RM_PAREN(T)>::BuildArrays( \
    const vtkm::cont::ArrayHandle<VTK_M_RM_PAREN(T)>& keys,                                        \
    vtkm::worklet::KeysSortType sort,                                                              \
    vtkm::cont::DeviceAdapterId device);                                                           \
  template VTKM_WORKLET_EXPORT VTKM_CONT void vtkm::worklet::Keys<VTK_M_RM_PAREN(T)>::BuildArrays( \
    const vtkm::cont::ArrayHandleVirtual<VTK_M_RM_PAREN(T)>& keys,                                 \
    vtkm::worklet::KeysSortType sort,                                                              \
    vtkm::cont::DeviceAdapterId device);                                                           \
  template VTKM_WORKLET_EXPORT VTKM_CONT void vtkm::worklet::Keys<VTK_M_RM_PAREN(                  \
    T)>::BuildArraysInPlace(vtkm::cont::ArrayHandle<VTK_M_RM_PAREN(T)>& keys,                      \
                            vtkm::worklet::KeysSortType sort,                                      \
                            vtkm::cont::DeviceAdapterId device);                                   \
  extern template VTKM_WORKLET_EXPORT VTKM_CONT void vtkm::worklet::Keys<VTK_M_RM_PAREN(           \
    T)>::BuildArraysInPlace(vtkm::cont::ArrayHandleVirtual<VTK_M_RM_PAREN(T)>& keys,               \
                            vtkm::worklet::KeysSortType sort,                                      \
                            vtkm::cont::DeviceAdapterId device)

#define VTK_M_KEYS_EXPORT(T)                                                                       \
  VTK_M_KEYS_EXPORT_TYPE((T));                                                                     \
  VTK_M_KEYS_EXPORT_TYPE((vtkm::Vec<T, 2>));                                                       \
  VTK_M_KEYS_EXPORT_TYPE((vtkm::Vec<T, 3>));                                                       \
  VTK_M_KEYS_EXPORT_TYPE((vtkm::Vec<T, 4>))

VTK_M_KEYS_EXPORT(char);
VTK_M_KEYS_EXPORT(vtkm::Int8);
VTK_M_KEYS_EXPORT(vtkm::UInt8);
VTK_M_KEYS_EXPORT(vtkm::Int16);
VTK_M_KEYS_EXPORT(vtkm::UInt16);
VTK_M_KEYS_EXPORT(vtkm::Int32);
VTK_M_KEYS_EXPORT(vtkm::UInt32);
VTK_M_KEYS_EXPORT(vtkm::Int64);
VTK_M_KEYS_EXPORT(vtkm::UInt64);
VTK_M_KEYS_EXPORT(vtkm::Float32);
VTK_M_KEYS_EXPORT(vtkm::Float64);

#undef VTK_M_KEYS_EXPORT
#undef VTK_M_KEYS_EXPORT_TYPE
#undef VTK_M_RM_PAREN
