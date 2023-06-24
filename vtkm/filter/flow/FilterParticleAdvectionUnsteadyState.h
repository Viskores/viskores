//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_FilterParticleAdvectionUnsteadyState_h
#define vtk_m_filter_flow_FilterParticleAdvectionUnsteadyState_h

#include <vtkm/filter/flow/FilterParticleAdvection.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

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
struct FlowTraits;

template <typename Derived>
class VTKM_FILTER_FLOW_EXPORT FilterParticleAdvectionUnsteadyState : public FilterParticleAdvection
{
public:
  using ParticleType = typename FlowTraits<Derived>::ParticleType;
  using FieldType = typename FlowTraits<Derived>::FieldType;
  using TerminationType = typename FlowTraits<Derived>::TerminationType;
  using AnalysisType = typename FlowTraits<Derived>::AnalysisType;

  VTKM_CONT void SetPreviousTime(vtkm::FloatDefault t1) { this->Time1 = t1; }

  VTKM_CONT void SetNextTime(vtkm::FloatDefault t2) { this->Time2 = t2; }

  VTKM_CONT void SetNextDataSet(const vtkm::cont::DataSet& ds) { this->Input2 = { ds }; }

  VTKM_CONT void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->Input2 = pds; }

private:
  VTKM_CONT FieldType GetField(const vtkm::cont::DataSet& data) const
  {
    const Derived* inst = static_cast<const Derived*>(this);
    return inst->GetField(data);
  }

  VTKM_CONT TerminationType GetTermination(const vtkm::cont::DataSet& data) const
  {
    const Derived* inst = static_cast<const Derived*>(this);
    return inst->GetTermination(data);
  }

  VTKM_CONT AnalysisType GetAnalysis(const vtkm::cont::DataSet& data) const
  {
    const Derived* inst = static_cast<const Derived*>(this);
    return inst->GetAnalysis(data);
  }

  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
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

  vtkm::cont::PartitionedDataSet Input2;
  vtkm::FloatDefault Time1 = -1;
  vtkm::FloatDefault Time2 = -1;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_FilterParticleAdvectionUnsteadyState_h
