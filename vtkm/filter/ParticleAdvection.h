//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ParticleAdvection_h
#define vtk_m_filter_ParticleAdvection_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/flow/ParticleAdvection.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED(
  1.8,
  "Use vtkm/filter/flow/ParticleAdvection.h instead of vtkm/filter/ParticleAdvection.h")
inline void ParticleAdvection_deprecated() {}

inline void ParticleAdvection_deprecated_warning()
{
  ParticleAdvection_deprecated();
}

}
}

#endif //vtk_m_filter_ParticleAdvection_h
