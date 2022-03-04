//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_WarpVector_h
#define vtk_m_filter_WarpVector_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/field_transform/WarpVector.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/field_transform/WarpVector.h instead of vtkm/filter/WarpVector.h.")
inline void WarpVector_deprecated() {}

inline void WarpVector_deprecated_warning()
{
  WarpVector_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_WarpVector_h
