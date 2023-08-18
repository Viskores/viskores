//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/FilterParticleAdvectionSteadyState.h>

#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegratorSteadyState.h>
#include <vtkm/filter/flow/internal/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

template <typename Derived>
VTKM_CONT typename FilterParticleAdvectionSteadyState<Derived>::FieldType
FilterParticleAdvectionSteadyState<Derived>::GetField(const vtkm::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetField(data);
}

template <typename Derived>
VTKM_CONT typename FilterParticleAdvectionSteadyState<Derived>::TerminationType
FilterParticleAdvectionSteadyState<Derived>::GetTermination(const vtkm::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetTermination(data);
}

template <typename Derived>
VTKM_CONT typename FilterParticleAdvectionSteadyState<Derived>::AnalysisType
FilterParticleAdvectionSteadyState<Derived>::GetAnalysis(const vtkm::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetAnalysis(data);
}

template <typename Derived>
VTKM_CONT vtkm::cont::PartitionedDataSet
FilterParticleAdvectionSteadyState<Derived>::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  //using ParticleType    = FilterParticleAdvectionSteadyState<Derived>::ParticleType;
  //using FieldType       = FilterParticleAdvectionSteadyState<Derived>::FieldType;
  //using TerminationType = FilterParticleAdvectionSteadyState<Derived>::TerminationType;
  //using AnalysisType    = FilterParticleAdvectionSteadyState<Derived>::AnalysisType;
  using DSIType = vtkm::filter::flow::internal::
    DataSetIntegratorSteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;

  this->ValidateOptions();


  vtkm::filter::flow::internal::BoundsMap boundsMap(input);
  std::vector<DSIType> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto dataset = input.GetPartition(i);

    // Build the field for the current dataset
    FieldType field = this->GetField(dataset);
    // Build the termination for the current dataset
    TerminationType termination = this->GetTermination(dataset);
    // Build the analysis for the current dataset
    AnalysisType analysis = this->GetAnalysis(dataset);

    dsi.emplace_back(blockId, field, dataset, this->SolverType, termination, analysis);
  }

  vtkm::filter::flow::internal::ParticleAdvector<DSIType> pav(
    boundsMap, dsi, this->UseThreadedAlgorithm, this->UseAsynchronousCommunication);

  vtkm::cont::ArrayHandle<ParticleType> particles;
  this->Seeds.AsArrayHandle(particles);
  return pav.Execute(particles, this->StepSize);
}

}
}
} // namespace vtkm::filter::flow

#include <vtkm/filter/flow/ParticleAdvection.h>
#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/filter/flow/WarpXStreamline.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

template class FilterParticleAdvectionSteadyState<vtkm::filter::flow::ParticleAdvection>;
template class FilterParticleAdvectionSteadyState<vtkm::filter::flow::Streamline>;
template class FilterParticleAdvectionSteadyState<vtkm::filter::flow::WarpXStreamline>;

} // namespace flow
} // namespace filter
} // namespace vtkm
