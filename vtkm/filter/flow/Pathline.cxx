//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/Pathline.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT Pathline::FieldType Pathline::GetField(const vtkm::cont::DataSet& dataset) const
{
  const auto& fieldNm = this->GetActiveFieldName();
  if (!dataset.HasPointField(fieldNm) && !dataset.HasCellField(fieldNm))
    throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
  auto assoc = dataset.GetField(fieldNm).GetAssociation();
  ArrayType arr;
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(fieldNm).GetData(), arr);
  return vtkm::worklet::flow::VelocityField<ArrayType>(arr, assoc);
}

VTKM_CONT Pathline::TerminationType Pathline::GetTermination(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return Pathline::TerminationType(this->NumberOfSteps);
}

VTKM_CONT Pathline::AnalysisType Pathline::GetAnalysis(const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return Pathline::AnalysisType(this->NumberOfSteps);
}
//VTKM_CONT vtkm::filter::flow::FlowResultType Pathline::GetResultType() const
//{
//  return vtkm::filter::flow::FlowResultType::STREAMLINE_TYPE;
//}

}
}
} // namespace vtkm::filter::flow
