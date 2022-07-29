//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Contour_h
#define vtk_m_filter_Contour_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/contour/Contour.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/contour/Contour.h instead of vtkm/filter/Contour.h.")
inline void Contour_deprecated() {}

inline void Contour_deprecated_warning()
{
  Contour_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Contour_h
