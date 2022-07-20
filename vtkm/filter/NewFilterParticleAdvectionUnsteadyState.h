//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_NewFilterParticleAdvectionUnsteadyState_h
#define vtk_m_filter_NewFilterParticleAdvectionUnsteadyState_h

#include <vtkm/filter/NewFilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{

class NewFilterParticleAdvectionUnsteadyState : public NewFilterParticleAdvection
{
public:
  VTKM_CONT
  NewFilterParticleAdvectionUnsteadyState(
    vtkm::filter::particleadvection::ParticleAdvectionResultType rType)
    : NewFilterParticleAdvection(rType)
  {
  }

  void SetPreviousTime(vtkm::FloatDefault t1) { this->Time1 = t1; }
  void SetNextTime(vtkm::FloatDefault t2) { this->Time2 = t2; }
  void SetNextDataSet(const vtkm::cont::DataSet& ds) { this->Input2 = { ds }; }
  void SetNextDataSet(const vtkm::cont::PartitionedDataSet& pds) { this->Input2 = pds; }

protected:
  VTKM_CONT virtual void ValidateOptions() const override
  {
    this->NewFilterParticleAdvection::ValidateOptions();
    if (this->Time1 >= this->Time2)
      throw vtkm::cont::ErrorFilterExecution("PreviousTime must be less than NextTime");
  }

  VTKM_CONT
  std::vector<std::shared_ptr<vtkm::filter::particleadvection::DataSetIntegratorUnsteadyState>>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::particleadvection::BoundsMap& boundsMap) const
  {
    using DSIType = vtkm::filter::particleadvection::DataSetIntegratorUnsteadyState;

    std::string activeField = this->GetActiveFieldName();

    std::vector<std::shared_ptr<DSIType>> dsi;
    for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
    {
      vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
      auto ds1 = input.GetPartition(i);
      auto ds2 = this->Input2.GetPartition(i);
      if ((!ds1.HasPointField(activeField) && !ds1.HasCellField(activeField)) ||
          (!ds2.HasPointField(activeField) && !ds2.HasCellField(activeField)))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

      dsi.push_back(std::shared_ptr<DSIType>(new DSIType(ds1,
                                                         ds2,
                                                         this->Time1,
                                                         this->Time2,
                                                         blockId,
                                                         activeField,
                                                         this->SolverType,
                                                         this->VecFieldType,
                                                         this->ResultType)));
    }

    return dsi;
  }

  vtkm::cont::PartitionedDataSet Input2;
  vtkm::FloatDefault Time1;
  vtkm::FloatDefault Time2;
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_NewFilterParticleAdvectionUnsteadyState_h
