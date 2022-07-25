//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_flow_NewFilterParticleAdvectionSteadyState_h
#define vtk_m_filter_flow_NewFilterParticleAdvectionSteadyState_h

#include <vtkm/filter/flow/NewFilterParticleAdvection.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

class NewFilterParticleAdvectionSteadyState : public NewFilterParticleAdvection
{
public:
  VTKM_CONT
  NewFilterParticleAdvectionSteadyState() {}

protected:
  VTKM_CONT std::vector<vtkm::filter::flow::DataSetIntegratorSteadyState*> CreateDataSetIntegrators(
    const vtkm::cont::PartitionedDataSet& input,
    const vtkm::filter::flow::BoundsMap& boundsMap,
    const vtkm::filter::flow::FlowResultType& resultType) const
  {
    using DSIType = vtkm::filter::flow::DataSetIntegratorSteadyState;

    std::string activeField = this->GetActiveFieldName();

    std::vector<DSIType*> dsi;
    for (vtkm::Id i = 0; i < input.GetNumberOfPartitions(); i++)
    {
      vtkm::Id blockId = boundsMap.GetLocalBlockId(i);
      auto ds = input.GetPartition(i);
      if (!ds.HasPointField(activeField) && !ds.HasCellField(activeField))
        throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");

      dsi.push_back(
        new DSIType(ds, blockId, activeField, this->SolverType, this->VecFieldType, resultType));
    }

    return dsi;
  }
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_NewFilterParticleAdvectionSteadyState_h
