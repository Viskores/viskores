//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ClipWithField_h
#define vtk_m_filter_ClipWithField_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/contour/ClipWithField.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/contour/ClipWithField.h instead of vtkm/filter/ClipWithField.h.")
inline void ClipWithField_deprecated() {}

inline void ClipWithField_deprecated_warning()
{
  ClipWithField_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ClipWithField_h
