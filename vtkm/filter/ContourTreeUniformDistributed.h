//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ContourTreeUniformDistributed_h
#define vtk_m_filter_ContourTreeUniformDistributed_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/scalar_topology/ContourTreeUniformDistributed.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/scalar_topology/ContourTreeUniformDistributed.h instead of "
                "vtkm/filter/ContourTreeUniformDistributed.h.")
inline void ContourTreeUniformDistributed_deprecated() {}

inline void ContourTreeUniformDistributed_deprecated_warning()
{
  ContourTreeUniformDistributed_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ContourTreeUniformDistributed_h
