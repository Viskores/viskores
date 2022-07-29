//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ExtractStructured_h
#define vtk_m_filter_ExtractStructured_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/entity_extraction/ExtractStructured.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/entity_extraction/ExtractStructured.h instead of "
                "vtkm/filter/ExtractStructured.h.")
inline void ExtractStructured_deprecated() {}

inline void ExtractStructured_deprecated_warning()
{
  ExtractStructured_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ExtractStructured_h
