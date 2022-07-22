//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_NewFilterParticleAdvectionSteadyState_h
#define vtk_m_filter_NewFilterParticleAdvectionSteadyState_h

#include <vtkm/filter/flow/NewFilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{
class NewFilterParticleAdvectionSteadyState : public NewFilterParticleAdvection
{
public:
  VTKM_CONT
  NewFilterParticleAdvectionSteadyState(
    vtkm::filter::particleadvection::ParticleAdvectionResultType rType)
    : NewFilterParticleAdvection(rType)
  {
  }

protected:
  VTKM_CONT std::vector<vtkm::filter::particleadvection::DataSetIntegratorSteadyState*>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::particleadvection::BoundsMap& boundsMap) const
  {
    using DSIType = vtkm::filter::particleadvection::DataSetIntegratorSteadyState;

    std::string activeField = this->GetActiveFieldName();

    std::vector<DSIType*> dsi;
    for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
    {
      vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
      auto ds = input.GetPartition(i);
      if (!ds.HasPointField(activeField) && !ds.HasCellField(activeField))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

      dsi.push_back(new DSIType(
        ds, blockId, activeField, this->SolverType, this->VecFieldType, this->ResultType));
    }

    return dsi;
  }
};

}
} // namespace vtkm::filter

#endif // vtk_m_filter_NewFilterParticleAdvectionSteadyState_h
