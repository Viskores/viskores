//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_field_transform_RGBToLab_h
#define viskores_filter_field_transform_RGBToLab_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/field_transform/viskores_filter_field_transform_export.h>

namespace viskores
{
namespace filter
{
namespace field_transform
{

/// \brief Convert RGB colors to CIE L*a*b* colors.
///
/// This filter converts an RGB field to Lab color values using the standard sRGB
/// transfer function and the CIE D65 reference white. Input colors may be
/// represented as `viskores::Vec3ui_8` byte RGB values or as normalized
/// floating-point RGB triplets (`viskores::Vec3f_32` or `viskores::Vec3f_64`).
///
/// If no output field name is specified, the output name is set to the input name
/// with `_lab` attached to it.
///
class VISKORES_FILTER_FIELD_TRANSFORM_EXPORT RGBToLab : public viskores::filter::Filter
{
public:
  /// @brief Specify whether this filter should ignore the alpha channel.
  ///
  /// If this option is set to true (the default), then when this filter operates
  /// on a 4-component field, the 4th component is treated as an alpha channel
  /// and ignored. That is the alpha channel is ignored and the resulting field
  /// has the standard 3 Lab components. If this option is set to false and a
  /// field with 4 components is provided, a `viskores::cont::ErrorBadType` is
  /// thrown.
  void SetIgnoreAlpha(bool flag) { this->IgnoreAlpha = flag; }

  /// @copydoc SetIgnoreAlpha
  bool GetIgnoreAlpha() const { return this->IgnoreAlpha; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  bool IgnoreAlpha = true;
};

} // namespace field_transform
} // namespace filter
} // namespace viskores

#endif // viskores_filter_field_transform_RGBToLab_h
