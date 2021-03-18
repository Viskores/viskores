//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ImageDifference_hxx
#define vtk_m_filter_ImageDifference_hxx

#include <vtkm/filter/ImageDifference.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/worklet/AveragePointNeighborhood.h>
#include <vtkm/worklet/ImageDifference.h>

namespace vtkm
{
namespace filter
{

namespace detail
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
} // namespace detail

inline VTKM_CONT ImageDifference::ImageDifference()
  : vtkm::filter::FilterField<ImageDifference>()
  , AverageRadius(0)
  , PixelShiftRadius(0)
  , AllowedPixelErrorRatio(0.00025f)
  , PixelDiffThreshold(0.05f)
  , ImageDiffWithinThreshold(true)
  , SecondaryFieldName("image-2")
  , SecondaryFieldAssociation(vtkm::cont::Field::Association::ANY)
  , ThresholdFieldName("threshold-output")
{
  this->SetPrimaryField("image-1");
  this->SetOutputFieldName("image-diff");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ImageDifference::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& primary,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  this->ImageDiffWithinThreshold = true;
  if (!fieldMetadata.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Performing Image Difference");

  vtkm::cont::Field secondaryField;
  secondaryField = input.GetField(this->SecondaryFieldName, this->SecondaryFieldAssociation);
  auto secondary = vtkm::filter::ApplyPolicyFieldOfType<T>(secondaryField, policy, *this);

  auto cellSet = vtkm::filter::ApplyPolicyCellSetStructured(input.GetCellSet(), policy, *this);
  vtkm::cont::ArrayHandle<T, StorageType> diffOutput;
  vtkm::cont::ArrayHandle<T, StorageType> primaryOutput;
  vtkm::cont::ArrayHandle<T, StorageType> secondaryOutput;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> thresholdOutput;

  if (this->AverageRadius > 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Performing Average with radius: " << this->AverageRadius);
    auto averageWorklet = vtkm::worklet::AveragePointNeighborhood(this->AverageRadius);
    this->Invoke(averageWorklet, cellSet, primary, primaryOutput);
    this->Invoke(averageWorklet, cellSet, secondary, secondaryOutput);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Not performing average");
    primaryOutput = primary;
    secondaryField.GetData().AsArrayHandle(secondaryOutput);
  }

  if (this->PixelShiftRadius > 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image in Neighborhood");
    auto diffWorklet =
      vtkm::worklet::ImageDifferenceNeighborhood(this->PixelShiftRadius, this->PixelDiffThreshold);
    this->Invoke(diffWorklet, cellSet, primaryOutput, secondaryOutput, diffOutput, thresholdOutput);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image directly");
    auto diffWorklet = vtkm::worklet::ImageDifference();
    this->Invoke(diffWorklet, primaryOutput, secondaryOutput, diffOutput, thresholdOutput);
  }


  vtkm::cont::ArrayHandle<vtkm::FloatDefault> errorPixels;
  vtkm::cont::Algorithm::CopyIf(thresholdOutput,
                                thresholdOutput,
                                errorPixels,
                                detail::GreaterThanThreshold(this->PixelDiffThreshold));
  if (errorPixels.GetNumberOfValues() >
      thresholdOutput.GetNumberOfValues() * this->AllowedPixelErrorRatio)
  {
    this->ImageDiffWithinThreshold = false;
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Difference within threshold: "
               << this->ImageDiffWithinThreshold
               << ", for pixels outside threshold: " << errorPixels.GetNumberOfValues()
               << ", with a total number of pixesl: " << thresholdOutput.GetNumberOfValues()
               << ", and an allowable pixel error ratio: " << this->AllowedPixelErrorRatio
               << ", with a total summed threshold error: "
               << vtkm::cont::Algorithm::Reduce(errorPixels, static_cast<FloatDefault>(0)));

  vtkm::cont::DataSet clone;
  clone.CopyStructure(input);
  clone.AddField(fieldMetadata.AsField(this->GetOutputFieldName(), diffOutput));
  clone.AddField(fieldMetadata.AsField(this->GetThresholdFieldName(), thresholdOutput));

  VTKM_ASSERT(clone.HasField(this->GetOutputFieldName(), fieldMetadata.GetAssociation()));
  VTKM_ASSERT(clone.HasField(this->GetThresholdFieldName(), fieldMetadata.GetAssociation()));

  return clone;
}

} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_ImageDifference_hxx
