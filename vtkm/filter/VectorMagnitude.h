//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_VectorMagnitude_h
#define vtk_m_filter_VectorMagnitude_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/vector_analysis/VectorMagnitude.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/vector_analysis/VectorMagnitude.h instead of vtkm/filter/VectorMagnitude.h.")
inline void VectorMagnitude_deprecated() {}

inline void VectorMagnitude_deprecated_warning()
{
  VectorMagnitude_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_VectorMagnitude_h
