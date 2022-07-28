//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_NewFilterParticleAdvectionUnsteadyState_h
#define vtk_m_filter_flow_NewFilterParticleAdvectionUnsteadyState_h

#include <vtkm/filter/flow/BoundsMap.h>
#include <vtkm/filter/flow/NewFilterParticleAdvection.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

// Forward declaration
class DataSetIntegratorUnsteadyState;

class VTKM_FILTER_FLOW_EXPORT NewFilterParticleAdvectionUnsteadyState
  : public NewFilterParticleAdvection
{
public:
  VTKM_CONT void SetPreviousTime(vtkm::FloatDefault t1) { this->Time1 = t1; }

  VTKM_CONT void SetNextTime(vtkm::FloatDefault t2) { this->Time2 = t2; }

  VTKM_CONT void SetNextDataSet(const vtkm::cont::DataSet& ds) { this->Input2 = { ds }; }

  VTKM_CONT void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->Input2 = pds; }

protected:
  VTKM_CONT virtual void ValidateOptions() const override
  {
    this->NewFilterParticleAdvection::ValidateOptions();
    if (this->Time1 >= this->Time2)
      throw vtkm::cont::ErrorFilterExecution("PreviousTime must be less than NextTime");
  }

  VTKM_CONT std::vector<vtkm::filter::flow::DataSetIntegratorUnsteadyState>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::flow::BoundsMap& boundsMap,
                           const vtkm::filter::flow::FlowResultType& resultType) const;

private:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  vtkm::cont::PartitionedDataSet Input2;
  vtkm::FloatDefault Time1 = -1;
  vtkm::FloatDefault Time2 = -1;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_NewFilterParticleAdvectionUnsteadyState_h
