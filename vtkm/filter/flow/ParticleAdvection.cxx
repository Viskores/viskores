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
#include <vtkm/filter/flow/ParticleAdvection.h>

#include <vtkm/filter/flow/DataSetIntegratorSteadyState.h>
#include <vtkm/filter/flow/ParticleAdvectionTypes.h>
#include <vtkm/filter/flow/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT vtkm::cont::PartitionedDataSet ParticleAdvection::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::flow::DataSetIntegratorSteadyState;
  this->ValidateOptions();

  vtkm::filter::flow::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap);

  vtkm::filter::flow::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->ResultType);

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
}
} // namespace vtkm::filter::flow
