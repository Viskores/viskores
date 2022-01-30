//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_SurfaceNormal_h
#define vtk_m_filter_SurfaceNormal_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/vector_calculus/SurfaceNormal.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/vector_calculus/SurfaceNormal.h instead of vtkm/filter/SurfaceNormal.h.")
inline void SurfaceNormal_deprecated() {}

inline void SurfaceNormal_deprecated_warning()
{
  SurfaceNormal_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::vector_calculus::SurfaceNormal.") SurfaceNormal
  : public vtkm::filter::vector_calculus::SurfaceNormal
{
  using vector_calculus::SurfaceNormal::SurfaceNormal;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_SurfaceNormal_h
