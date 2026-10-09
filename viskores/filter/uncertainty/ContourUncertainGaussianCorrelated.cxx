//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/uncertainty/ContourUncertainGaussianCorrelated.h>

#include <viskores/cont/ErrorBadValue.h>
#include <viskores/filter/uncertainty/ContourUncertainMAGICCorrelatedClosedForm.h>
#include <viskores/filter/uncertainty/ContourUncertainMAGICCorrelatedMonteCarlo.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{
namespace
{

template <typename FilterType>
void ConfigureImplementation(const ContourUncertainGaussianCorrelated& source, FilterType& target)
{
  for (viskores::Id isoIndex = 0; isoIndex < source.GetNumberOfIsoValues(); ++isoIndex)
  {
    target.SetIsoValue(isoIndex, source.GetIsoValue(isoIndex));
  }
  target.SetMeanField(source.GetActiveFieldName(0));
  target.SetVarianceField(source.GetActiveFieldName(1));
  target.SetRhoXField(source.GetActiveFieldName(2));
  target.SetRhoYField(source.GetActiveFieldName(3));
  target.SetRhoZField(source.GetActiveFieldName(4));
  target.SetFieldsToPass(source.GetFieldsToPass());
  target.SetCrossingVarianceName(source.GetCrossingVarianceName());
  target.SetExpectedCrossingName(source.GetExpectedCrossingName());
  target.SetGenerateNormals(source.GetGenerateNormals());
  target.SetComputeFastNormals(source.GetComputeFastNormals());
  target.SetNormalArrayName(source.GetNormalArrayName());
  target.SetMergeDuplicatePoints(source.GetMergeDuplicatePoints());
  target.SetInputCellDimension(source.GetInputCellDimension());
}

} // anonymous namespace

viskores::cont::DataSet ContourUncertainGaussianCorrelated::DoExecute(
  const viskores::cont::DataSet& input)
{
  if (this->Approach == ApproachEnum::ClosedForm)
  {
    ContourUncertainMAGICCorrelatedClosedForm filter;
    ConfigureImplementation(*this, filter);
    return filter.Execute(input);
  }
  if (this->Approach == ApproachEnum::MonteCarlo)
  {
    ContourUncertainMAGICCorrelatedMonteCarlo filter;
    ConfigureImplementation(*this, filter);
    filter.SetNumberOfSamples(this->NumberOfSamples);
    return filter.Execute(input);
  }

  throw viskores::cont::ErrorBadValue("Unsupported approach for ContourUncertainGaussianCorrelated.");
}

} // namespace uncertainty
} // namespace filter
} // namespace viskores
