//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ExtractGeometry_h
#define vtk_m_filter_ExtractGeometry_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/entity_extraction/ExtractGeometry.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/entity_extraction/ExtractGeometry.h instead of vtkm/filter/ExtractGeometry.h.")
inline void ExtractGeometry_deprecated() {}

inline void ExtractGeometry_deprecated_warning()
{
  ExtractGeometry_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ExtractGeometry_h
