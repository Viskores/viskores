//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ContourTreeUniform_h
#define vtk_m_filter_ContourTreeUniform_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/scalar_topology/ContourTreeUniform.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/scalar_topology/ContourTreeUniform.h instead of "
                "vtkm/filter/ContourTreeUniform.h.")
inline void ContourTreeUniform_deprecated() {}

inline void ContourTreeUniform_deprecated_warning()
{
  ContourTreeUniform_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ContourTreeUniform_h
