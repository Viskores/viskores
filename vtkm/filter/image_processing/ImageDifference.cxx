//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/UncertainCellSet.h>

#include <vtkm/filter/image_processing/ImageDifference.h>
#include <vtkm/filter/image_processing/worklet/ImageDifference.h>
#include <vtkm/worklet/AveragePointNeighborhood.h>

namespace vtkm
{
namespace filter
{
namespace image_processing
{
namespace
{
struct GreaterThanThreshold
{
  GreaterThanThreshold(const vtkm::FloatDefault& thresholdError)
    : ThresholdError(thresholdError)
  {
  }
  VTKM_EXEC_CONT bool operator()(const vtkm::FloatDefault& x) const { return x > ThresholdError; }
  vtkm::FloatDefault ThresholdError;
};
} // anonymous namespace

VTKM_CONT ImageDifference::ImageDifference()
{
  this->SetPrimaryField("image-1");
  this->SetSecondaryField("image-2");
  this->SetOutputFieldName("image-diff");
}

VTKM_CONT vtkm::cont::DataSet ImageDifference::DoExecute(const vtkm::cont::DataSet& input)
{
  this->ImageDiffWithinThreshold = true;

  const auto& primaryField = this->GetFieldFromDataSet(input);
  if (!primaryField.IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Performing Image Difference");

  auto inputCellSet = input.GetCellSet().ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED>();

  const auto& secondaryField = this->GetFieldFromDataSet(1, input);

  vtkm::cont::UnknownArrayHandle diffOutput;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> thresholdOutput;

  auto resolveType = [&](const auto& primaryArray) {
    // use std::decay to remove const ref from the decltype of primaryArray.
    using T = typename std::decay_t<decltype(primaryArray)>::ValueType;
    vtkm::cont::ArrayHandle<T> secondaryArray;
    vtkm::cont::ArrayCopyShallowIfPossible(secondaryField.GetData(), secondaryArray);

    vtkm::cont::ArrayHandle<T> primaryOutput;
    vtkm::cont::ArrayHandle<T> secondaryOutput;
    if (this->AverageRadius > 0)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Performing Average with radius: " << this->AverageRadius);
      auto averageWorklet = vtkm::worklet::AveragePointNeighborhood(this->AverageRadius);
      this->Invoke(averageWorklet, inputCellSet, primaryArray, primaryOutput);
      this->Invoke(averageWorklet, inputCellSet, secondaryArray, secondaryOutput);
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Not performing average");
      vtkm::cont::ArrayCopyShallowIfPossible(primaryArray, primaryOutput);
      secondaryOutput = secondaryArray;
    }

    vtkm::cont::ArrayHandle<T> diffArray;
    if (this->PixelShiftRadius > 0)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image in Neighborhood");
      this->Invoke(vtkm::worklet::ImageDifferenceNeighborhood(this->PixelShiftRadius,
                                                              this->PixelDiffThreshold),
                   inputCellSet,
                   primaryOutput,
                   secondaryOutput,
                   diffArray,
                   thresholdOutput);
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image directly");
      this->Invoke(vtkm::worklet::ImageDifference(),
                   primaryOutput,
                   secondaryOutput,
                   diffArray,
                   thresholdOutput);
    }
    diffOutput = diffArray;
  };
  this->CastAndCallVecField<4>(primaryField, resolveType);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> errorPixels;
  vtkm::cont::Algorithm::CopyIf(
    thresholdOutput, thresholdOutput, errorPixels, GreaterThanThreshold(this->PixelDiffThreshold));
  if (errorPixels.GetNumberOfValues() >
      thresholdOutput.GetNumberOfValues() * this->AllowedPixelErrorRatio)
  {
    this->ImageDiffWithinThreshold = false;
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Difference within threshold: "
               << this->ImageDiffWithinThreshold
               << ", for pixels outside threshold: " << errorPixels.GetNumberOfValues()
               << ", with a total number of pixels: " << thresholdOutput.GetNumberOfValues()
               << ", and an allowable pixel error ratio: " << this->AllowedPixelErrorRatio
               << ", with a total summed threshold error: "
               << vtkm::cont::Algorithm::Reduce(errorPixels, static_cast<FloatDefault>(0)));

  auto outputDataSet = this->CreateResultFieldPoint(input, this->GetOutputFieldName(), diffOutput);
  outputDataSet.AddPointField(this->GetThresholdFieldName(), thresholdOutput);
  return outputDataSet;
}
}
} // namespace filter
} // namespace vtkm
