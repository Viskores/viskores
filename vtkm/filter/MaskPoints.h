//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_MaskPoints_h
#define vtk_m_filter_MaskPoints_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/entity_extraction/MaskPoints.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/entity_extraction/MaskPoints.h instead of vtkm/filter/MaskPoints.h.")
inline void MaskPoints_deprecated() {}

inline void MaskPoints_deprecated_warning()
{
  MaskPoints_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_MaskPoints_h
