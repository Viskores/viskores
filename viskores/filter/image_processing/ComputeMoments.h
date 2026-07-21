//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_image_processing_ComputeMoments_h
#define viskores_filter_image_processing_ComputeMoments_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/image_processing/viskores_filter_image_processing_export.h>

namespace viskores
{
namespace filter
{
namespace image_processing
{

/// \brief Computes spatial moments of image point fields.
///
/// `ComputeMoments` evaluates local image moments for the active point field on
/// a structured data set. For each point, samples within the configured
/// neighborhood radius are integrated using the configured physical spacing.
/// The filter computes all moment terms up to the configured order and adds
/// them as point fields to the output data set.
///
/// The active field can be scalar or vector-valued. Vector components are
/// processed component-wise in the generated moment fields.
class VISKORES_FILTER_IMAGE_PROCESSING_EXPORT ComputeMoments : public viskores::filter::Filter
{
public:
  /// @brief Construct a filter that computes zeroth-order moments.
  ///
  /// The default radius is 1, the default spacing is `(1, 1, 1)`, and the
  /// default maximum order is 0.
  VISKORES_CONT ComputeMoments();

  /// @brief Set the physical radius of the neighborhood used for each moment.
  ///
  /// The radius is converted to an index-space radius using the configured
  /// spacing. Samples outside the radius are not included in the moment.
  VISKORES_CONT void SetRadius(double _radius) { this->Radius = _radius; }

  /// @brief Set the physical spacing between image samples.
  ///
  /// The spacing is used to convert the radius to structured index space and to
  /// scale the computed moment integrals.
  VISKORES_CONT void SetSpacing(viskores::Vec3f _spacing) { this->Spacing = _spacing; }

  /// @brief Set the maximum moment order to compute.
  ///
  /// The filter computes all moment terms from order 0 through the given order.
  VISKORES_CONT void SetOrder(viskores::Int32 _order) { this->Order = _order; }

private:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;

  double Radius = 1;
  viskores::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  viskores::Int32 Order = 0;
};
} // namespace image_processing
} // namespace filter
} // namespace viskores

#endif //viskores_filter_image_processing_ComputeMoments_h
