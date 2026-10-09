//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//  This code is based on the MAGIC algorithm:
//  Athawale, T., Moreland, K., Pugmire, D., Johnson, C., Rosen, P.,
//  Norman, M., Georgiadou, A., Entezari, A. (2025). MAGIC: Marching Cubes
//  Isosurface Uncertainty Visualization for Gaussian Uncertain Data with
//  Spatial Correlation.

#include <viskores/filter/uncertainty/ContourUncertainMAGICCorrelatedMonteCarlo.h>

#include <viskores/cont/ArrayHandleRandomStandardNormal.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/uncertainty/worklet/gaussian/ContourUncertainGaussianCorrelated.h>
#include <viskores/filter/uncertainty/worklet/gaussian/InterpolateFieldWorklet.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{

viskores::cont::DataSet ContourUncertainMAGICCorrelatedMonteCarlo::DoExecute(
  const viskores::cont::DataSet& input)
{
  viskores::cont::CellSetStructured<3> cellSet;
  input.GetCellSet().AsCellSet(cellSet);
  viskores::Id3 resolution = cellSet.GetPointDimensions();

  viskores::filter::contour::Contour contourExtract;
  contourExtract.SetIsoValues(this->IsoValues);
  contourExtract.SetGenerateNormals(this->GetGenerateNormals());
  contourExtract.SetComputeFastNormals(this->GetComputeFastNormals());
  contourExtract.SetNormalArrayName(this->GetNormalArrayName());
  contourExtract.SetMergeDuplicatePoints(this->GetMergeDuplicatePoints());
  contourExtract.SetInputCellDimension(this->GetInputCellDimension());
  contourExtract.SetActiveField(0, this->GetActiveFieldName(0), this->GetActiveFieldAssociation());

  viskores::filter::FieldSelection fieldSelection = this->GetFieldsToPass();
  for (viskores::IdComponent fieldId = 0; fieldId < input.GetNumberOfFields(); ++fieldId)
  {
    viskores::cont::Field field = input.GetField(fieldId);
    if (field.IsPointField())
    {
      fieldSelection.AddField(field, viskores::filter::FieldSelection::Mode::Exclude);
    }
  }
  fieldSelection.AddField(this->GetActiveFieldName(0),
                          this->GetActiveFieldAssociation(0),
                          viskores::filter::FieldSelection::Mode::Select);
  contourExtract.SetFieldsToPass(fieldSelection);
  contourExtract.SetAddInterpolationEdgeIds(true);

  viskores::cont::DataSet contours = contourExtract.Execute(input);
  viskores::cont::UnknownArrayHandle outputVariance;
  viskores::cont::ArrayHandle<viskores::Float64> expectedCrossings;
  viskores::cont::ArrayHandleRandomStandardNormal<viskores::Float64> randomSamples(
    this->NumberOfSamples * 2);

  auto resolveArray = [&](auto inputVarianceArray)
  {
    using T = typename std::decay_t<decltype(inputVarianceArray)>::ValueType;
    viskores::cont::ArrayHandle<T> variance;
    viskores::worklet::detail::ComputeEdgeVarianceCorrelatedMonteCarlo(
      inputVarianceArray,
      this->GetFieldFromDataSet(2, input).GetData(),
      this->GetFieldFromDataSet(3, input).GetData(),
      this->GetFieldFromDataSet(4, input).GetData(),
      this->GetFieldFromDataSet(0, contours).GetData(),
      this->GetFieldFromDataSet(0, input).GetData(),
      contours.GetField("edgeIds").GetData(),
      randomSamples,
      this->NumberOfSamples,
      resolution,
      variance,
      expectedCrossings);
    outputVariance = variance;
  };
  this->CastAndCallScalarField(this->GetFieldFromDataSet(1, input), resolveArray);

  auto interpField = [&](const auto& inputArray, viskores::cont::UnknownArrayHandle& outputArray)
  {
    outputArray = viskores::worklet::detail::InterpolateField(
      inputArray, contours.GetField("edgeIds").GetData(), expectedCrossings);
  };
  const std::string rhoXName = this->GetActiveFieldName(2);
  const std::string rhoYName = this->GetActiveFieldName(3);
  const std::string rhoZName = this->GetActiveFieldName(4);
  auto mapField = [&](viskores::cont::DataSet& outputData, const viskores::cont::Field& inputField)
  {
    const std::string& fieldName = inputField.GetName();
    if (fieldName == rhoXName || fieldName == rhoYName || fieldName == rhoZName)
    {
      return;
    }
    if (inputField.IsPointField())
    {
      viskores::cont::UnknownArrayHandle outputArray;
      this->CastAndCallVariableVecField(inputField.GetData(), interpField, outputArray);
      outputData.AddPointField(inputField.GetName(), outputArray);
    }
    else
    {
      outputData.AddField(inputField);
    }
  };
  viskores::cont::DataSet outputData = this->CreateResult(input, contours.GetCellSet(), mapField);
  outputData.AddPointField(this->GetCrossingVarianceName(), outputVariance);
  outputData.AddPointField(this->GetExpectedCrossingName(), expectedCrossings);
  return outputData;
}

} // namespace uncertainty
} // namespace filter
} // namespace viskores
