//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/XGCPoincare.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT XGCPoincare::FieldType XGCPoincare::GetField(const vtkm::cont::DataSet& dataset) const
{
  const auto& field1 = this->GetActiveFieldName(0);
  const auto& field2 = this->GetActiveFieldName(1);
  const auto& field3 = this->GetActiveFieldName(2);
  const auto& field4 = this->GetActiveFieldName(3);
  const auto& field5 = this->GetActiveFieldName(4);
  const auto& field6 = this->GetActiveFieldName(5);


  /*
  XGCField(const FieldComponentType& as_ff,
           const FieldVecType& _dAs_ff_rzp,
           const FieldComponentType& coeff_1D,
           const FieldComponentType& coeff_2D,
           const FieldComponentType& psi,
           const FieldVecType& _B_rzp,
           const XGCParams& params,
           bool useDeltaBScale,
           bool useBScale,
           const Association assoc)
*/

  CompArrayType as_ff;
  CompArrayType coeff_1D;
  CompArrayType coeff_2D;
  CompArrayType psi;
  VecArrayType dAs_ff_rzp;
  VecArrayType b_rzp;
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field1).GetData(), as_ff);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field2).GetData(), dAs_ff_rzp);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field3).GetData(), coeff_1D);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field4).GetData(), coeff_2D);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field5).GetData(), b_rzp);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field6).GetData(), psi);

  vtkm::worklet::flow::XGCParams params = this->GetXGCParams();
  vtkm::FloatDefault period = this->GetPeriod();

  return XGCPoincare::FieldType(
    as_ff, dAs_ff_rzp, coeff_1D, coeff_2D, b_rzp, psi, params, period, false, 1.0, false, 1.0);
}

VTKM_CONT XGCPoincare::TerminationType XGCPoincare::GetTermination(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  return XGCPoincare::TerminationType(this->NumberOfSteps, this->MaxPunctures);
}

VTKM_CONT XGCPoincare::AnalysisType XGCPoincare::GetAnalysis(
  const vtkm::cont::DataSet& dataset) const
{
  // dataset not used
  (void)dataset;
  CompArrayType coeff_1D;
  CompArrayType coeff_2D;
  const auto& field3 = this->GetActiveFieldName(2);
  const auto& field4 = this->GetActiveFieldName(3);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field3).GetData(), coeff_1D);
  vtkm::cont::ArrayCopyShallowIfPossible(dataset.GetField(field4).GetData(), coeff_2D);
  vtkm::worklet::flow::XGCParams params = this->GetXGCParams();
  vtkm::FloatDefault period = this->GetPeriod();
  return XGCPoincare::AnalysisType(
    this->NumberOfSteps, this->MaxPunctures, coeff_1D, coeff_2D, params, period);
}

}
}
} // namespace vtkm::filter::flow
