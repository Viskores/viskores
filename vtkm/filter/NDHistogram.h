//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_NDHistogram_h
#define vtk_m_filter_NDHistogram_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/density_estimate/NDHistogram.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/density_estimate/NDHistogram.h instead of vtkm/filter/NDHistogram.h.")
inline void NDHistogram_deprecated() {}

inline void NDHistogram_deprecated_warning()
{
  NDHistogram_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_NDHistogram_h
