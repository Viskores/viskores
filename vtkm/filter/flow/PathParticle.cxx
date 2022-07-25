//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/flow/BoundsMap.h>
#include <vtkm/filter/flow/PathParticle.h>

#include <vtkm/filter/flow/DataSetIntegratorUnsteadyState.h>
#include <vtkm/filter/flow/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

//vtkm::filter::flow::FlowResultType PathParticle::ResultType = vtkm::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE;


VTKM_CONT vtkm::cont::PartitionedDataSet PathParticle::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::flow::DataSetIntegratorUnsteadyState;
  this->ValidateOptions();

  vtkm::filter::flow::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap, this->ResultType);

  vtkm::filter::flow::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->ResultType);

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
}
} // namespace vtkm::filter::flow
