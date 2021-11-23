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

#include <vtkm/Particle.h>
#include <vtkm/filter/FilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{
/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

template <typename ParticleType = vtkm::Particle>
class ParticleAdvectionBase
  : public vtkm::filter::FilterParticleAdvection<ParticleAdvectionBase<ParticleType>, ParticleType>
{
public:
  VTKM_CONT ParticleAdvectionBase();

  template <typename DerivedPolicy>
  vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);
};

using ParticleAdvection = ParticleAdvectionBase<vtkm::Particle>;
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_ParticleAdvection_hxx
#include <vtkm/filter/ParticleAdvection.hxx>
#endif

#endif // vtk_m_filter_ParticleAdvection_h
