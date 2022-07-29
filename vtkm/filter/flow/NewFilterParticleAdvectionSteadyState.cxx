//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/NewFilterParticleAdvectionSteadyState.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/flow/DataSetIntegratorSteadyState.h>
#include <vtkm/filter/flow/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT std::vector<vtkm::filter::flow::DataSetIntegratorSteadyState>
NewFilterParticleAdvectionSteadyState::CreateDataSetIntegrators(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::flow::BoundsMap& boundsMap,
  const vtkm::filter::flow::FlowResultType& resultType) const
{
  using DSIType = vtkm::filter::flow::DataSetIntegratorSteadyState;

  std::string activeField = this->GetActiveFieldName();

  std::vector<DSIType> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds = input.GetPartition(i);
    if (!ds.HasPointField(activeField) && !ds.HasCellField(activeField))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

    dsi.emplace_back(ds, blockId, activeField, this->SolverType, this->VecFieldType, resultType);
  }

  return dsi;
}

VTKM_CONT vtkm::cont::PartitionedDataSet NewFilterParticleAdvectionSteadyState::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::flow::DataSetIntegratorSteadyState;
  this->ValidateOptions();

  vtkm::filter::flow::BoundsMap boundsMap(input);
  auto dsi = this->CreateDataSetIntegrators(input, boundsMap, this->GetResultType());

  vtkm::filter::flow::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->GetResultType());

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
}
} // namespace vtkm::filter::flow
