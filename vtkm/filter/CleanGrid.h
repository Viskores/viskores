//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CleanGrid_h
#define vtk_m_filter_CleanGrid_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/clean_grid/CleanGrid.h instead of vtkm/filter/CleanGrid.h.")
inline void CleanGrid_deprecated() {}

inline void CleanGrid_deprecated_warning()
{
  CleanGrid_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_CleanGrid_h
