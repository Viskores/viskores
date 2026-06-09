//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_image_processing_SSIM_h
#define viskores_filter_image_processing_SSIM_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/image_processing/viskores_filter_image_processing_export.h>

namespace viskores
{
namespace filter
{
namespace image_processing
{

/// \brief Computes the Structural Similarity Index Measure (SSIM) between two image fields.
///
/// This filter compares the active primary and secondary fields with the
/// standard SSIM formula over a local patch around each point:
/// `((2*mean_x*mean_y + c1) * (2*cov_xy + c2)) /
///  ((mean_x^2 + mean_y^2 + c1) * (var_x + var_y + c2))`.
///
/// The primary and secondary fields must be point fields on a structured data
/// set. `PatchRadius()` controls the radius of the local patch used for each
/// output point. Scalar fields are compared directly. Color or vector fields are
/// compared by flattening their components within each local patch. Neighboring
/// samples within the patch are weighted by a Gaussian based on index distance.
///
/// The result is a `viskores::cont::DataSet` containing a point field named
/// "ssim" by default. Use `ComputeMetric()` to get the average of the pointwise
/// SSIM values.
///
/// This filter is similar in design to VTK's `vtkImageSSIM` filter with some
/// differences that may result in differences in the computation. Both filters
/// use a window, but this filter applies Gaussian weights similar to that in the
/// Wang et al. 2004 paper whereas the VTK implementation uses a flat window.
///
/// This implementation also uses the one pass identity of E[X²] - E[X]² to
/// compute variance rather than taking a second pass to compute sum(w(x_i - μ)²).
/// These are equivalent for small values (well below 10^6), which is fine
/// for image data. There may be precision problems for general scientific data.
class VISKORES_FILTER_IMAGE_PROCESSING_EXPORT SSIM : public viskores::filter::Filter
{
public:
  /// @brief Construct an SSIM filter.
  ///
  /// The default primary field name is "image-1", the default secondary field
  /// name is "image-2", the default output field name is "ssim", and the
  /// default patch radius is 3.
  VISKORES_CONT SSIM();

  /// @brief Choose the primary field to compare.
  ///
  /// The primary field must have the same association, number of values, and
  /// number of components as the secondary field.
  VISKORES_CONT
  void SetPrimaryField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(name, association);
  }

  /// @brief Get the name of the primary field.
  VISKORES_CONT const std::string& GetPrimaryFieldName() const
  {
    return this->GetActiveFieldName();
  }

  /// @brief Get the association used to select the primary field.
  VISKORES_CONT viskores::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }

  /// @brief Choose the secondary field to compare.
  ///
  /// The secondary field must have the same association, number of values, and
  /// number of components as the primary field.
  VISKORES_CONT
  void SetSecondaryField(
    const std::string& name,
    viskores::cont::Field::Association association = viskores::cont::Field::Association::Any)
  {
    this->SetActiveField(1, name, association);
  }

  /// @brief Get the name of the secondary field.
  VISKORES_CONT const std::string& GetSecondaryFieldName() const
  {
    return this->GetActiveFieldName(1);
  }

  /// @brief Get the association used to select the secondary field.
  VISKORES_CONT viskores::cont::Field::Association GetSecondaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation(1);
  }

  /// @brief Enumeration of modes to select the dynamic range.
  ///
  /// The SSIM algorithm operates based on the range of _possible_ values that
  /// can be held by either field. This range can either be guessed or set
  /// explicitly. This enumerator is used to select how the dynamic range is
  /// specified.
  enum struct DynamicRangeType
  {
    /// @copydoc SetDynamicRangeToDefault
    Default,
    /// @copydoc SetDynamicRangeToExplicit
    Explicit,
    /// @copydoc SetDynamicRangeToRange
    Range
  };

  /// @brief Use a default value for the dynamic range based on the field type.
  ///
  /// If the field type is an integer, the dynamic range will be set to the range
  /// of possible values for that integer (e.g., 255 for 8-bit integers or 65535
  /// for 16-bit integers). If the field type is a float, the dynamic range will
  /// be set to the computed range of the input fields. If that computed range is
  /// zero, the dynamic range falls back to 1.0. This is the default behavior of
  /// the SSIM filter.
  VISKORES_CONT void SetDynamicRangeToDefault()
  {
    this->SetDynamicRangeType(DynamicRangeType::Default);
  }

  /// @brief Use an explicit value for the dynamic range.
  ///
  /// This behavior for the dynamic range requires the user to set an explicit
  /// value for the range. This value is set with the
  /// `viskores::filter::image_processing::SetDynamicRange()` method.
  VISKORES_CONT void SetDynamicRangeToExplicit()
  {
    this->SetDynamicRangeType(DynamicRangeType::Explicit);
  }

  /// @brief Use the range of the input fields for the dynamic range.
  ///
  /// This behavior for the dynamic range queries the input fields for the range
  /// they contain. This range is used as the dynamic range. Note that this
  /// assumes that the fields contain the minimum and maximum possible values,
  /// which is not always the case.
  VISKORES_CONT void SetDynamicRangeToRange()
  {
    this->SetDynamicRangeType(DynamicRangeType::Range);
  }

  /// @brief The method for specifying the dynamic range of the field values.
  ///
  /// The SSIM algorithm operates based on the range of _possible_ values that
  /// can be held by either field. This range can either be guessed or set
  /// explicitly and the type specifies how this is determined.
  VISKORES_CONT void SetDynamicRangeType(DynamicRangeType rangeType)
  {
    this->RangeType = rangeType;
  }
  /// @copydoc SetDynamicRangeType
  VISKORES_CONT DynamicRangeType GetDynamicRangeType() const { return this->RangeType; }

  /// @brief Set the dynamic range used to compute SSIM constants.
  ///
  /// The SSIM algorithm operates based on the range of _possible_ values that
  /// can be held by either field. Calling this method sets what this range is.
  /// Calling this method changes the dynamic range type to `Explicit`.
  VISKORES_CONT void SetDynamicRange(viskores::Float64 dynamicRange)
  {
    this->DynamicRange = dynamicRange;
    this->SetDynamicRangeToExplicit();
  }

  /// @brief Get the dynamic range used to compute SSIM constants.
  VISKORES_CONT viskores::Float64 GetDynamicRange() const { return this->DynamicRange; }

  /// @brief The K1 SSIM stabilization constant.
  ///
  /// The `K1` constant is a small number used by SSIM to stabilize the metric
  /// for dark, flat, or low-contrast areas of the image and to prevent division
  /// by zero. The default value for `K1` is 0.01, which is customary for SSIM
  /// calculations.
  VISKORES_CONT void SetK1(viskores::Float64 k1) { this->K1 = k1; }

  /// @copydoc SetK1
  VISKORES_CONT viskores::Float64 GetK1() const { return this->K1; }

  /// @brief Set the K2 SSIM stabilization constant.
  ///
  /// The `K2` constant is a small number used by SSIM to stabilize the metric
  /// for low-contrast or low-variance areas of the image and to prevent division
  /// by zero. The default value for `K2` is 0.03, which is customary for SSIM
  /// calculations.
  VISKORES_CONT void SetK2(viskores::Float64 k2) { this->K2 = k2; }

  /// @copydoc SetK2
  VISKORES_CONT viskores::Float64 GetK2() const { return this->K2; }

  /// @brief Set the radius of the local SSIM patch.
  ///
  /// A radius of 0 uses only the point itself. A positive radius includes
  /// neighboring points within a circular or spherical patch in structured
  /// image-index space. The default radius is 3.
  VISKORES_CONT void SetPatchRadius(viskores::IdComponent patchRadius)
  {
    this->PatchRadius = patchRadius;
  }

  /// @brief Get the radius of the local SSIM patch.
  VISKORES_CONT viskores::IdComponent GetPatchRadius() const { return this->PatchRadius; }

  /// @brief Compute and return the SSIM value for the configured fields.
  ///
  /// This method computes the local SSIM point field and returns the average of
  /// those values without creating an output `viskores::cont::DataSet`.
  VISKORES_CONT viskores::Float64 ComputeMetric(const viskores::cont::DataSet& input) const;

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  VISKORES_CONT viskores::Float64 GetRealDynamicRange(
    const viskores::cont::Field& primaryField,
    const viskores::cont::Field& secondaryField) const;

  DynamicRangeType RangeType = DynamicRangeType::Default;
  viskores::Float64 DynamicRange = 1.0;
  viskores::Float64 K1 = 0.01;
  viskores::Float64 K2 = 0.03;
  viskores::IdComponent PatchRadius = 3;
};

} // namespace image_processing
} // namespace filter
} // namespace viskores

#endif // viskores_filter_image_processing_SSIM_h
