//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Tetrahedralize_h
#define vtk_m_filter_Tetrahedralize_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/geometry_refinement/Tetrahedralize.h instead of vtkm/filter/Tetrahedralize.h.")
inline void Tetrahedralize_deprecated() {}

inline void Tetrahedralize_deprecated_warning()
{
  Tetrahedralize_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Tetrahedralize_h
