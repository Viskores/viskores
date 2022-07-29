//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ThresholdPoints_h
#define vtk_m_filter_ThresholdPoints_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/entity_extraction/ThresholdPoints.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/entity_extraction/ThresholdPoints.h instead of vtkm/filter/ThresholdPoints.h.")
inline void ThresholdPoints_deprecated() {}

inline void ThresholdPoints_deprecated_warning()
{
  ThresholdPoints_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ThresholdPoints_h
