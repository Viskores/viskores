//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Lagrangian_h
#define vtk_m_filter_Lagrangian_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/flow/Lagrangian.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/flow/Lagrangian.h instead of vtkm/filter/Lagrangian.h")
inline void Lagrangian_deprecated() {}

inline void Lagrangian_deprecated_warning()
{
  Lagrangian_deprecated();
}

}
}

#endif //vtk_m_filter_Lagrangian_h
