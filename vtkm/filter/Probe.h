//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Probe_h
#define vtk_m_filter_Probe_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/resampling/Probe.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8, "Use vtkm/filter/resampling/Probe.h instead of vtkm/filter/Probe.h.")
inline void Probe_deprecated() {}

inline void Probe_deprecated_warning()
{
  Probe_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Probe_h
