//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Gradient_h
#define vtk_m_filter_Gradient_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/vector_analysis/Gradient.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/vector_analysis/Gradient.h instead of vtkm/filter/Gradient.h.")
inline void Gradient_deprecated() {}

inline void Gradient_deprecated_warning()
{
  Gradient_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Gradient_h
