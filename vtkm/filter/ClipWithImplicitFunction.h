//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ClipWithImplicitFunction_h
#define vtk_m_filter_ClipWithImplicitFunction_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/contour/ClipWithImplicitFunction.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/contour/ClipWithImplicitFunction.h instead of "
                "vtkm/filter/ClipWithImplicitFunction.h.")
inline void ClipWithImplicitFunction_deprecated() {}

inline void ClipWithImplicitFunction_deprecated_warning()
{
  ClipWithImplicitFunction_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ClipWithImplicitFunction_h
