//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_MeshQuality_h
#define vtk_m_filter_MeshQuality_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/mesh_info/MeshQuality.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/mesh_info/MeshQuality.h instead of vtkm/filter/MeshQuality.h.")
inline void MeshQuality_deprecated() {}

inline void MeshQuality_deprecated_warning()
{
  MeshQuality_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_MeshQuality_h
