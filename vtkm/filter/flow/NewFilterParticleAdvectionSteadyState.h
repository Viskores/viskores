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

#include <vtkm/filter/flow/FlowTypes.h>
#include <vtkm/filter/flow/NewFilterParticleAdvection.h>
#include <vtkm/filter/flow/internal/BoundsMap.h>
#include <vtkm/filter/flow/internal/DataSetIntegratorSteadyState.h>

#include <vtkm/filter/flow/vtkm_filter_flow_export.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

// Forward declaration
class DataSetIntegratorSteadyState;

class VTKM_FILTER_FLOW_EXPORT NewFilterParticleAdvectionSteadyState
  : public NewFilterParticleAdvection
{
protected:
  VTKM_CONT std::vector<vtkm::filter::flow::internal::DataSetIntegratorSteadyState>
  CreateDataSetIntegrators(const vtkm::cont::PartitionedDataSet& input,
                           const vtkm::filter::flow::internal::BoundsMap& boundsMap,
                           const vtkm::filter::flow::FlowResultType& resultType) const;

private:
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;
};

}
}
} // namespace vtkm::filter::flow

#endif // vtk_m_filter_flow_NewFilterParticleAdvectionSteadyState_h
