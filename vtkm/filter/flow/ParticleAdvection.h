//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_ParticleAdvection_h
#define vtk_m_filter_flow_ParticleAdvection_h

#include <vtkm/Particle.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/NewFilterParticleAdvectionSteadyState.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

class VTKM_FILTER_FLOW_EXPORT ParticleAdvection
  : public vtkm::filter::flow::NewFilterParticleAdvectionSteadyState
{
public:
private:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  const vtkm::filter::flow::FlowResultType ResultType =
    vtkm::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_ParticleAdvection_h
