//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_GenerateIds_h
#define vtk_m_filter_GenerateIds_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/field_transform/GenerateIds.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/field_transform/GenerateIds.h instead of vtkm/filter/GenerateIds.h.")
inline void GenerateIds_deprecated() {}

inline void GenerateIds_deprecated_warning()
{
  GenerateIds_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_GenerateIds_h
