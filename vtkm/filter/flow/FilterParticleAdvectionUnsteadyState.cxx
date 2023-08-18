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

#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegratorUnsteadyState.h>
#include <vtkm/filter/flow/internal/ParticleAdvector.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

template <typename Derived>
VTKM_CONT typename FilterParticleAdvectionUnsteadyState<Derived>::FieldType
FilterParticleAdvectionUnsteadyState<Derived>::GetField(const vtkm::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetField(data);
}

template <typename Derived>
VTKM_CONT typename FilterParticleAdvectionUnsteadyState<Derived>::TerminationType
FilterParticleAdvectionUnsteadyState<Derived>::GetTermination(const vtkm::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetTermination(data);
}

template <typename Derived>
VTKM_CONT typename FilterParticleAdvectionUnsteadyState<Derived>::AnalysisType
FilterParticleAdvectionUnsteadyState<Derived>::GetAnalysis(const vtkm::cont::DataSet& data) const
{
  const Derived* inst = static_cast<const Derived*>(this);
  return inst->GetAnalysis(data);
}

template <typename Derived>
VTKM_CONT vtkm::cont::PartitionedDataSet
FilterParticleAdvectionUnsteadyState<Derived>::DoExecutePartitions(
  const vtkm::cont::PartitionedDataSet& input)
{
  this->ValidateOptions();

  using DSIType = vtkm::filter::flow::internal::
    DataSetIntegratorUnsteadyState<ParticleType, FieldType, TerminationType, AnalysisType>;

  vtkm::filter::flow::internal::BoundsMap boundsMap(input);

  std::vector<DSIType> dsi;
  for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
  {
    vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
    auto ds1 = input.GetPartition(i);
    auto ds2 = this->Input2.GetPartition(i);

    // Build the field for the current dataset
    FieldType field1 = this->GetField(ds1);
    FieldType field2 = this->GetField(ds2);

    // Build the termination for the current dataset
    TerminationType termination = this->GetTermination(ds1);

    AnalysisType analysis = this->GetAnalysis(ds1);

    dsi.emplace_back(blockId,
                     field1,
                     field2,
                     ds1,
                     ds2,
                     this->Time1,
                     this->Time2,
                     this->SolverType,
                     termination,
                     analysis);
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

#include <vtkm/filter/flow/PathParticle.h>
#include <vtkm/filter/flow/Pathline.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

template class FilterParticleAdvectionUnsteadyState<vtkm::filter::flow::PathParticle>;
template class FilterParticleAdvectionUnsteadyState<vtkm::filter::flow::Pathline>;

} // namespace flow
} // namespace filter
} // namespace vtkm
