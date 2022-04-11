//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ParticleDensityNearestGridPoint_h
#define vtk_m_filter_ParticleDensityNearestGridPoint_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/density_estimate/ParticleDensityNearestGridPoint.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/density_estimate/ParticleDensityNearestGridPoint.h instead of "
                "vtkm/filter/ParticleDensityNearestGridPoint.h.")
inline void ParticleDensityNearestGridPoint_deprecated() {}

inline void ParticleDensityNearestGridPoint_deprecated_warning()
{
  ParticleDensityNearestGridPoint_deprecated();
}

}
} // namespace vtkm::filter

#endif //vtk_m_filter_ParticleDensityNearestGridPoint_h
