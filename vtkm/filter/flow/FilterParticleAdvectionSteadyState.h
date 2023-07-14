//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_FilterParticleAdvectionSteadyState_h
#define vtk_m_filter_flow_FilterParticleAdvectionSteadyState_h

#include <vtkm/filter/flow/FilterParticleAdvection.h>
#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

template <typename Derived>
struct FlowTraits;

template <typename Derived>
class VTKM_FILTER_FLOW_EXPORT FilterParticleAdvectionSteadyState : public FilterParticleAdvection
{
public:
  using ParticleType = typename FlowTraits<Derived>::ParticleType;
  using FieldType = typename FlowTraits<Derived>::FieldType;
  using TerminationType = typename FlowTraits<Derived>::TerminationType;
  using AnalysisType = typename FlowTraits<Derived>::AnalysisType;

private:
  VTKM_CONT FieldType GetField(const vtkm::cont::DataSet& data) const;

  VTKM_CONT TerminationType GetTermination(const vtkm::cont::DataSet& data) const;

  VTKM_CONT AnalysisType GetAnalysis(const vtkm::cont::DataSet& data) const;

  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input) override;
};
}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_FilterParticleAdvectionSteadyState_h
