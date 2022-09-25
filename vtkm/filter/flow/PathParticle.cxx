//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/PathParticle.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT PathParticle::FieldType PathParticle::GetField(const vtkm::cont::DataSet& dataset) const
{
  const auto& fieldNm = this->GetActiveFieldName();
  if (!dataset.HasPointField(fieldNm) && !dataset.HasCellField(fieldNm))
    throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
  auto assoc = dataset.GetField(fieldNm).GetAssociation();
  ArrayType arr;
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(fieldNm).GetData(), arr);
  return PathParticle::FieldType(arr, assoc);
}

VTKM_CONT PathParticle::TerminationType PathParticle::GetTermination(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return PathParticle::TerminationType(this->NumberOfSteps);
}

VTKM_CONT PathParticle::AnalysisType PathParticle::GetAnalysis(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return PathParticle::AnalysisType();
}

//VTKM_CONT vtkm::filter::flow::FlowResultType PathParticle::GetResultType() const
//{
//  return vtkm::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE;
//}

}
}
} // namespace vtkm::filter::flow
