//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Threshold_h
#define vtk_m_filter_Threshold_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/entity_extraction/Threshold.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/entity_extraction/Threshold.h instead of vtkm/filter/Threshold.h.")
inline void Threshold_deprecated() {}

inline void Threshold_deprecated_warning()
{
  Threshold_deprecated();
}

class VTKM_DEPRECATED(1.8, "Use vtkm::filter::entity_extraction::Threshold.") Threshold
  : public vtkm::filter::entity_extraction::Threshold
{
  using entity_extraction::Threshold::Threshold;
};

}
} // namespace vtkm::filter

#endif //vtk_m_filter_Threshold_h
