//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ParticleDensityCloudInCell_h
#define vtk_m_filter_ParticleDensityCloudInCell_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/density_estimate/ParticleDensityCloudInCell.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/density_estimate/ParticleDensityCloudInCell.h instead of "
                "vtkm/filter/ParticleDensityCloudInCell.h.")
inline void ParticleDensityCloudInCell_deprecated() {}

inline void ParticleDensityCloudInCell_deprecated_warning()
{
  ParticleDensityCloudInCell_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ParticleDensityCloudInCell_h
