//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef viskores_filter_image_processing_ImageDifference_h
#define viskores_filter_image_processing_ImageDifference_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/image_processing/viskores_filter_image_processing_export.h>

namespace viskores
{
namespace filter
{
namespace image_processing
{
/// \brief Computes per-pixel differences between two image fields.
///
/// `ImageDifference` compares a primary image field A with a secondary image
/// field B. The output contains a point field named "image-diff" by default
/// with the per-pixel difference values A - B, and a threshold point field named
/// "threshold-output" by default with the magnitude of each pixel difference.
///
/// The filter can optionally smooth both input fields before comparison and can
/// optionally search a local pixel-shift neighborhood for the closest matching
/// primary pixel. `GetImageDiffWithinThreshold()` reports whether the number of
/// pixels outside the configured difference threshold is within the allowed
/// error ratio.
///
class VISKORES_FILTER_IMAGE_PROCESSING_EXPORT ImageDifference : public viskores::filter::Filter
{
public:
  /// @brief Construct an image-difference filter.
  ///
  /// The default primary field name is "image-1", the default secondary field
  /// name is "image-2", and the default output field name is "image-diff".
  VISKORES_CONT ImageDifference();

  /// @brief Get the radius used to average both images before comparison.
  VISKORES_CONT viskores::IdComponent GetAverageRadius() const { return this->AverageRadius; }

  /// @brief Set the radius used to average both images before comparison.
  ///
  /// A radius of 0 disables averaging. Positive values apply a point
  /// neighborhood average to both the primary and secondary fields before
  /// computing differences.
  VISKORES_CONT void SetAverageRadius(const viskores::IdComponent& averageRadius)
  {
    this->AverageRadius = averageRadius;
  }

  /// @brief Get the radius of the pixel-shift search neighborhood.
  VISKORES_CONT viskores::IdComponent GetPixelShiftRadius() const { return this->PixelShiftRadius; }

  /// @brief Set the radius of the pixel-shift search neighborhood.
  ///
  /// A radius of 0 compares pixels directly. Positive values search the primary
  /// image in a local neighborhood around each secondary pixel and report the
  /// smallest difference found.
  VISKORES_CONT void SetPixelShiftRadius(const viskores::IdComponent& pixelShiftRadius)
  {
    this->PixelShiftRadius = pixelShiftRadius;
  }

  /// @brief Get the maximum allowed ratio of pixels outside the difference threshold.
  VISKORES_CONT viskores::FloatDefault GetAllowedPixelErrorRatio() const
  {
    return this->AllowedPixelErrorRatio;
  }

  /// @brief Set the maximum allowed ratio of pixels outside the difference threshold.
  ///
  /// After execution, `GetImageDiffWithinThreshold()` returns false if the
  /// number of pixels whose threshold value exceeds `PixelDiffThreshold` is
  /// greater than this ratio of the total number of pixels.
  VISKORES_CONT void SetAllowedPixelErrorRatio(const viskores::FloatDefault& pixelErrorRatio)
  {
    this->AllowedPixelErrorRatio = pixelErrorRatio;
  }

  /// @brief Get the pixel-difference threshold.
  VISKORES_CONT viskores::FloatDefault GetPixelDiffThreshold() const
  {
    return this->PixelDiffThreshold;
  }

  /// @brief Set the pixel-difference threshold.
  ///
  /// The threshold field stores the magnitude of each pixel difference. Pixels
  /// with values greater than this threshold are counted as error pixels.
  VISKORES_CONT void SetPixelDiffThreshold(const viskores::FloatDefault& threshold)
  {
    this->PixelDiffThreshold = threshold;
  }

  /// @brief Return whether the most recent execution stayed within the threshold settings.
  ///
  /// This value is updated by `Execute()`. It is true when the ratio of pixels
  /// whose threshold values exceed `PixelDiffThreshold` is no greater than
  /// `AllowedPixelErrorRatio`.
  VISKORES_CONT bool GetImageDiffWithinThreshold() const { return this->ImageDiffWithinThreshold; }

  /// @brief Set the name of the threshold output field.
  VISKORES_CONT void SetThresholdFieldName(const std::string& name)
  {
    this->ThresholdFieldName = name;
  }

  /// @brief Get the name of the threshold output field.
  VISKORES_CONT std::string GetThresholdFieldName() const { return this->ThresholdFieldName; }

  /// @brief Choose the primary field to compare.
  ///
  /// For image difference A - B, A is the primary field. The primary field must
  /// be a point field and must have the same value type and number of values as
  /// the secondary field.
  VISKORES_CONT
  void SetPrimaryField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(name, association);
  }

  /// @brief Get the name of the primary field.
  VISKORES_CONT std::string GetPrimaryFieldName() const { return this->GetActiveFieldName(); }

  /// @brief Get the association used to select the primary field.
  VISKORES_CONT viskores::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }

  /// @brief Choose the secondary field to compare.
  ///
  /// For image difference A - B, B is the secondary field. The secondary field
  /// must be a point field and must have the same value type and number of
  /// values as the primary field.
  VISKORES_CONT
  void SetSecondaryField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(1, name, association);
  }

  /// @brief Get the name of the secondary field.
  VISKORES_CONT std::string GetSecondaryFieldName() const { return this->GetActiveFieldName(1); }

  /// @brief Get the association used to select the secondary field.
  VISKORES_CONT viskores::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& primaryArray) override;

  viskores::IdComponent AverageRadius = 0;
  viskores::IdComponent PixelShiftRadius = 0;
  viskores::FloatDefault AllowedPixelErrorRatio = 0.00025f;
  viskores::FloatDefault PixelDiffThreshold = 0.05f;
  bool ImageDiffWithinThreshold = true;
  std::string ThresholdFieldName = "threshold-output";
};
} // namespace image_processing
} // namespace filter
} // namespace viskores

#endif // viskores_filter_image_processing_ImageDifference_h
