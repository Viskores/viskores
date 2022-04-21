//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_SurfaceNormals_h
#define vtk_m_filter_SurfaceNormals_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/vector_analysis/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/vector_analysis/SurfaceNormals.h instead of vtkm/filter/SurfaceNormals.h.")
inline void SurfaceNormals_deprecated() {}

inline void SurfaceNormals_deprecated_warning()
{
  SurfaceNormals_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_SurfaceNormals_h
