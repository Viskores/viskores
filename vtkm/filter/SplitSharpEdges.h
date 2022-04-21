//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_SplitSharpEdges_h
#define vtk_m_filter_SplitSharpEdges_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/geometry_refinement/SplitSharpEdges.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/geometry_refinement/SplitSharpEdges.h instead of vtkm/filter/SplitSharpEdges.h.")
inline void SplitSharpEdges_deprecated() {}

inline void SplitSharpEdges_deprecated_warning()
{
  SplitSharpEdges_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_SplitSharpEdges_h
