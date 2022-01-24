//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Histogram_h
#define vtk_m_filter_Histogram_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/density_estimate/Histogram.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/density_estimate/Histogram.h instead of vtkm/filter/Histogram.h.")
inline void Histogram_deprecated() {}

inline void Histogram_deprecated_warning()
{
  Histogram_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::density_estimate::Histogram.") Histogram
  : public vtkm::filter::density_estimate::Histogram
{
  using density_estimate::Histogram::Histogram;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Histogram_h
