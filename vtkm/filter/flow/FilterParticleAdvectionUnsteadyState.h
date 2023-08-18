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
  VTKM_CONT FieldType GetField(const vtkm::cont::DataSet& data) const;

  VTKM_CONT TerminationType GetTermination(const vtkm::cont::DataSet& data) const;

  VTKM_CONT AnalysisType GetAnalysis(const vtkm::cont::DataSet& data) const;

  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& input);

  vtkm::cont::PartitionedDataSet Input2;
  vtkm::FloatDefault Time1 = -1;
  vtkm::FloatDefault Time2 = -1;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_FilterParticleAdvectionUnsteadyState_h
