//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Pathline_h
#define vtk_m_filter_Pathline_h

#include <vtkm/Particle.h>
#include <vtkm/filter/flow/NewFilterParticleAdvectionUnsteadyState.h>
#include <vtkm/filter/flow/ParticleAdvectionTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
/// \brief Advect particles in a vector field.

/// Takes as input a vector field and seed locations and generates the
/// end points for each seed through the vector field.

class VTKM_FILTER_FLOW_EXPORT Pathline
  : public vtkm::filter::NewFilterParticleAdvectionUnsteadyState
{
public:
  VTKM_CONT Pathline()
    : NewFilterParticleAdvectionUnsteadyState(
        vtkm::filter::particleadvection::ParticleAdvectionResultType::STREAMLINE_TYPE)
  {
  }

protected:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_Pathline_h
