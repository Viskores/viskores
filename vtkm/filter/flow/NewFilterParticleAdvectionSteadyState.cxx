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
#include <vtkm/filter/flow/internal/DataSetIntegratorSteadyState.h>

#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

namespace
{
VTKM_CONT std::vector<vtkm::filter::flow::internal::DataSetIntegratorSteadyState>
CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                         const std::pair<std::string, std::string>& activeField,
                         const vtkm::filter::flow::internal::BoundsMap& boundsMap,
                         const vtkm::filter::flow::IntegrationSolverType solverType,
                         const vtkm::filter::flow::VectorFieldType vecFieldType,
                         const vtkm::filter::flow::FlowResultType resultType)
{
  using DSIType = vtkm::filter::flow::internal::DataSetIntegratorSteadyState;

  std::vector<DSIType> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds = input.GetPartition(i);
    if (!ds.HasPointField(activeField.first) && !ds.HasCellField(activeField.first))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
    if (!ds.HasPointField(activeField.second) && !ds.HasCellField(activeField.second))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

    dsi.emplace_back(ds, blockId, activeField, solverType, vecFieldType, resultType);
  }

  return dsi;
}

} //anonymous namespace

VTKM_CONT vtkm::cont::PartitionedDataSet NewFilterParticleAdvectionSteadyState::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::flow::internal::DataSetIntegratorSteadyState;
  this->ValidateOptions();

  std::pair<std::string, std::string> field("E", "B");

  vtkm::filter::flow::internal::BoundsMap boundsMap(input);
  auto dsi = CreateDataSetIntegrators(
    input, field, boundsMap, this->SolverType, this->VecFieldType, this->GetResultType());

  vtkm::filter::flow::internal::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->GetResultType());

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
}
} // namespace vtkm::filter::flow
