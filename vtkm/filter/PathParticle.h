//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_PathParticle_h
#define vtk_m_filter_PathParticle_h

#include <vtkm/Particle.h>
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/NewFilterParticleAdvection.h>
#include <vtkm/filter/particleadvection/ParticleAdvectionTypes.h>

namespace vtkm
{
namespace filter
{
/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

class PathParticle : public vtkm::filter::NewFilterUnsteadyStateParticleAdvection
{
public:
  VTKM_CONT PathParticle()
    : NewFilterUnsteadyStateParticleAdvection(
        vtkm::filter::particleadvection::ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
  }

protected:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;
};

}
} // namespace vtkm::filter

#ifndef vtk_m_filter_PathParticle_hxx
#include <vtkm/filter/PathParticle.hxx>
#endif

#endif // vtk_m_filter_PathParticle_h
