//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_GhostCellClassify_h
#define vtk_m_filter_GhostCellClassify_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/mesh_info/GhostCellClassify.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/mesh_info/GhostCellClassify.h instead of vtkm/filter/GhostCellClassify.h.")
inline void GhostCellClassify_deprecated() {}

inline void GhostCellClassify_deprecated_warning()
{
  GhostCellClassify_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_GhostCellClassify_h
