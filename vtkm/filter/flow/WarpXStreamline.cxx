//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/WarpXStreamline.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT WarpXStreamline::FieldType WarpXStreamline::GetField(
  const vtkm::cont::DataSet& dataset) const
{
  const auto& electric = this->GetEField();
  const auto& magnetic = this->GetBField();
  if (!dataset.HasPointField(electric) && !dataset.HasCellField(electric))
    throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
  if (!dataset.HasPointField(magnetic) && !dataset.HasCellField(magnetic))
    throw vtkm::cont::ErrorFilterExecution("Unsupported field assocation");
  auto eAssoc = dataset.GetField(electric).GetAssociation();
  auto bAssoc = dataset.GetField(magnetic).GetAssociation();
  if (eAssoc != bAssoc)
  {
    throw vtkm::cont::ErrorFilterExecution("E and B field need to have same association");
  }
  ArrayType eField, bField;
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(electric).GetData(), eField);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(magnetic).GetData(), bField);
  return vtkm::worklet::flow::ElectroMagneticField<ArrayType>(eField, bField, eAssoc);
}

VTKM_CONT WarpXStreamline::TerminationType WarpXStreamline::GetTermination(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return WarpXStreamline::TerminationType(this->NumberOfSteps);
}

VTKM_CONT WarpXStreamline::AnalysisType WarpXStreamline::GetAnalysis(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return WarpXStreamline::AnalysisType(this->NumberOfSteps);
}

}
}
} // namespace vtkm::filter::flow
