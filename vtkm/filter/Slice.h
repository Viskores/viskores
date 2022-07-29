//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Slice_h
#define vtk_m_filter_Slice_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/contour/Slice.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/contour/Slice.h instead of vtkm/filter/Slice.h.")
inline void Slice_deprecated() {}

inline void Slice_deprecated_warning()
{
  Slice_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Slice_h
