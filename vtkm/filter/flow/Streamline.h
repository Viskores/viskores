//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_Streamline_h
#define vtk_m_filter_flow_Streamline_h

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

class VTKM_FILTER_FLOW_EXPORT Streamline
  : public vtkm::filter::flow::NewFilterParticleAdvectionSteadyState
{
public:
  VTKM_CONT Streamline()
    : NewFilterParticleAdvectionSteadyState(vtkm::filter::flow::FlowResultType::STREAMLINE_TYPE)
  {
  }

protected:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_Streamline_h
