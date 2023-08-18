//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/ParticleAdvection.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

// using ParticleType    = vtkm::Particle;
// using TerminationType = vtkm::worklet::flow::NormalTermination;
// using AnalysisType    = vtkm::worklet::flow::Streamline;
// using ArrayType       = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
// using FieldType       = vtkm::worklet::flow::VelocityField<ArrayType>;

VTKM_CONT ParticleAdvection::FieldType ParticleAdvection::GetField(
  const vtkm::cont::DataSet& dataset) const
{
  const auto& fieldNm = this->GetActiveFieldName();
  if (!dataset.HasPointField(fieldNm) && !dataset.HasCellField(fieldNm))
    throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
  auto assoc = dataset.GetField(fieldNm).GetAssociation();
  ArrayType arr;
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(fieldNm).GetData(), arr);
  return ParticleAdvection::FieldType(arr, assoc);
}

VTKM_CONT ParticleAdvection::TerminationType ParticleAdvection::GetTermination(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return ParticleAdvection::TerminationType(this->NumberOfSteps);
}

VTKM_CONT ParticleAdvection::AnalysisType ParticleAdvection::GetAnalysis(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return ParticleAdvection::AnalysisType{};
}

}
}
} // namespace vtkm::filter::flow
