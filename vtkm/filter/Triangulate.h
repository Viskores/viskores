//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Triangulate_h
#define vtk_m_filter_Triangulate_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/geometry_refinement/Triangulate.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/geometry_refinement/Triangulate.h instead of vtkm/filter/Triangulate.h.")
inline void Triangulate_deprecated() {}

inline void Triangulate_deprecated_warning()
{
  Triangulate_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Triangulate_h
