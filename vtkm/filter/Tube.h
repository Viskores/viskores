//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Tube_h
#define vtk_m_filter_Tube_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/geometry_refinement/Tube.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/geometry_refinement/Tube.h instead of vtkm/filter/Tube.h.")
inline void Tube_deprecated() {}

inline void Tube_deprecated_warning()
{
  Tube_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Tube_h
