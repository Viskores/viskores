//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_LagrangianStructures_h
#define vtk_m_filter_LagrangianStructures_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/flow/LagrangianStructures.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/flow/LagrangianStructures.h instead of vtkm/filter/LagrangianStructures.h")
inline void LagrangianStructures_deprecated() {}

inline void LagrangianStructures_deprecated_warning()
{
  LagrangianStructures_deprecated();
}

}
}

#endif //vtk_m_filter_LagrangnianStructures_h
