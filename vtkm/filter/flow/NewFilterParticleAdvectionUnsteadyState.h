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

#include <vtkm/filter/flow/NewFilterParticleAdvection.h>
#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

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

  VTKM_CONT std::vector<vtkm::filter::flow::DataSetIntegratorUnsteadyState*>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::flow::BoundsMap& boundsMap,
                           const vtkm::filter::flow::FlowResultType& resultType) const
  {
    using DSIType = vtkm::filter::flow::DataSetIntegratorUnsteadyState;

    std::string activeField = this->GetActiveFieldName();

    std::vector<DSIType*> dsi;
    for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
    {
      vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
      auto ds1 = input.GetPartition(i);
      auto ds2 = this->Input2.GetPartition(i);
      if ((!ds1.HasPointField(activeField) && !ds1.HasCellField(activeField)) ||
          (!ds2.HasPointField(activeField) && !ds2.HasCellField(activeField)))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

      dsi.push_back(new DSIType(ds1,
                                ds2,
                                this->Time1,
                                this->Time2,
                                blockId,
                                activeField,
                                this->SolverType,
                                this->VecFieldType,
                                resultType));
    }

    return dsi;
  }

  vtkm::cont::PartitionedDataSet Input2;
  vtkm::FloatDefault Time1 = -1;
  vtkm::FloatDefault Time2 = -1;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_NewFilterParticleAdvectionUnsteadyState_h
