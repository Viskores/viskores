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
VTK_M_KEYS_EXPORT(vtkm::Id);
VTK_M_KEYS_EXPORT(vtkm::Id3);
#ifdef VTKM_USE_64BIT_IDS
VTK_M_KEYS_EXPORT(vtkm::IdComponent);
#endif

#undef VTK_M_KEYS_EXPORT
