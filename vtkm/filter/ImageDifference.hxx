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

#include <vtkm/BinaryOperators.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/worklet/AveragePointNeighborhood.h>
#include <vtkm/worklet/ImageDifference.h>

namespace vtkm
{
namespace filter
{

inline VTKM_CONT ImageDifference::ImageDifference()
  : vtkm::filter::FilterField<ImageDifference>()
  , Radius(0)
  , Threshold(0.05f)
  , AveragePixels(false)
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
  vtkm::cont::ArrayHandle<vtkm::FloatDefault, StorageType> thresholdOutput;

  if (this->AveragePixels && this->Radius > 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Performing Average with radius: " << this->Radius);
    auto averageWorklet = vtkm::worklet::AveragePointNeighborhood(this->Radius);
    this->Invoke(averageWorklet, cellSet, primary, primaryOutput);
    this->Invoke(averageWorklet, cellSet, secondary, secondaryOutput);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Not performing average");
    primaryOutput = primary;
    secondaryOutput =
      secondaryField.GetData().template Cast<vtkm::cont::ArrayHandle<T, StorageType>>();
  }

  if (this->Radius > 0)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image in Neighborhood");
    auto diffWorklet = vtkm::worklet::ImageDifferenceNeighborhood(this->Radius, this->Threshold);
    this->Invoke(diffWorklet, cellSet, primaryOutput, secondaryOutput, diffOutput, thresholdOutput);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Diffing image directly");
    auto diffWorklet = vtkm::worklet::ImageDifference();
    this->Invoke(diffWorklet, primaryOutput, secondaryOutput, diffOutput, thresholdOutput);
  }

  // Dummy calculate the threshold.  If any value is greater than the min our images
  // are not similar enough.
  vtkm::FloatDefault maxThreshold =
    vtkm::cont::Algorithm::Reduce(thresholdOutput, vtkm::FloatDefault(0), vtkm::Maximum());
  if (maxThreshold > this->Threshold)
  {
    this->ImageDiffWithinThreshold = false;
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             "Difference within threshold: " << this->ImageDiffWithinThreshold);

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
