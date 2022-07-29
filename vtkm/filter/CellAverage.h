//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CellAverage_h
#define vtk_m_filter_CellAverage_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/field_conversion/CellAverage.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/field_conversion/CellAverage.h instead of vtkm/filter/CellAverage.h.")
inline void CellAverage_deprecated() {}

inline void CellAverage_deprecated_warning()
{
  CellAverage_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_CellAverage_h
