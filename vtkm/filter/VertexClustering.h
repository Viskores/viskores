//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_VertexClustering_h
#define vtk_m_filter_VertexClustering_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/geometry_refinement/VertexClustering.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/geometry_refinement/VertexClustering.h instead of "
                "vtkm/filter/VertexClustering.h.")
inline void VertexClustering_deprecated() {}

inline void VertexClustering_deprecated_warning()
{
  VertexClustering_deprecated();
}


}
} // namespace vtkm::filter

#endif //vtk_m_filter_VertexClustering_h
