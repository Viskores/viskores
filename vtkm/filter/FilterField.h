//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FilterField_h
#define vtk_m_filter_FilterField_h

#include <vtkm/filter/Filter.h>

#include <vtkm/Deprecated.h>


namespace vtkm
{
namespace filter
{

struct VTKM_DEPRECATED(
  2.2,
  "FilterField.h (and its class) are deprecated. Use Filter.h (and its class).")
  vtkm_filter_FilterField_h_deprecated
{
};

static vtkm_filter_FilterField_h_deprecated issue_deprecated_warning_filterfield()
{
  vtkm_filter_FilterField_h_deprecated x;
  return x;
}

} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_FilterField_h
