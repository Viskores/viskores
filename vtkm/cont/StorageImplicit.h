//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_StorageImplicit_h
#define vtk_m_cont_StorageImplicit_h

#include <vtkm/Deprecated.h>
#include <vtkm/cont/ArrayHandleImplicit.h>

namespace vtkm
{

VTKM_DEPRECATED(1.6, "Use ArrayHandleImplicit.h instead of StorageImplicit.h.")
inline void StorageImplicit_h_deprecated() {}

inline void ActivateStorageImplicit_h_deprecated_warning()
{
  StorageImplicit_h_deprecated();
}

} // namespace vtkm

#endif //vtk_m_cont_StorageImplicit_h
