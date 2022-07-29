//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_PointElevation_h
#define vtk_m_filter_PointElevation_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/field_transform/PointElevation.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/field_transform/PointElevation.h instead of vtkm/filter/PointElevation.h.")
inline void PointElevation_deprecated() {}

inline void PointElevation_deprecated_warning()
{
  PointElevation_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_PointElevation_h
