//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/FilterParticleAdvectionUnsteadyState.h>
#include <vtkm/filter/flow/internal/DataSetIntegratorUnsteadyState.h>
#include <vtkm/filter/flow/internal/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{
namespace
{
VTKM_CONT std::vector<vtkm::filter::flow::internal::DataSetIntegratorUnsteadyState>
CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                         const vtkm::cont::PartitionedDataSet& input2,
                         const std::string& activeField,
                         const vtkm::FloatDefault timer1,
                         const vtkm::FloatDefault timer2,
                         const vtkm::filter::flow::internal::BoundsMap& boundsMap,
                         const vtkm::filter::flow::IntegrationSolverType solverType,
                         const vtkm::filter::flow::VectorFieldType vecFieldType,
                         const vtkm::filter::flow::FlowResultType resultType)
{
  using DSIType = vtkm::filter::flow::internal::DataSetIntegratorUnsteadyState;

  std::vector<DSIType> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds1 = input.GetPartition(i);
    auto ds2 = input2.GetPartition(i);
    if ((!ds1.HasPointField(activeField) && !ds1.HasCellField(activeField)) ||
        (!ds2.HasPointField(activeField) && !ds2.HasCellField(activeField)))
      throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

    dsi.emplace_back(
      ds1, ds2, timer1, timer2, blockId, activeField, solverType, vecFieldType, resultType);
  }

  return dsi;
}
} // anonymous namespace

VTKM_CONT void FilterParticleAdvectionUnsteadyState::ValidateOptions() const
{
  this->FilterParticleAdvection::ValidateOptions();
  if (this->Time1 >= this->Time2)
    throw vtkm::cont::ErrorFilterExecution("PreviousTime must be less than NextTime");
}

VTKM_CONT vtkm::cont::PartitionedDataSet FilterParticleAdvectionUnsteadyState::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  using DSIType = vtkm::filter::flow::internal::DataSetIntegratorUnsteadyState;
  this->ValidateOptions();

  vtkm::filter::flow::internal::BoundsMap boundsMap(input);
  auto dsi = CreateDataSetIntegrators(input,
                                      this->Input2,
                                      this->GetActiveFieldName(),
                                      this->Time1,
                                      this->Time2,
                                      boundsMap,
                                      this->SolverType,
                                      this->VecFieldType,
                                      this->GetResultType());

  vtkm::filter::flow::internal::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->GetResultType());

  return pav.Execute(this->NumberOfSteps, this->StepSize, this->Seeds);
}

}
}
} // namespace vtkm::filter::flow
