//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_uncertainty_ContourUncertainGaussianIndependentClosedForm_h
#define viskores_filter_uncertainty_ContourUncertainGaussianIndependentClosedForm_h

#include <viskores/filter/contour/AbstractContour.h>
#include <viskores/filter/uncertainty/viskores_filter_uncertainty_export.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{

/// @brief Visualize isosurface uncertainty for independently Gaussian distributed data.
///
/// This filter computes the positional uncertainty of isosurfaces as a function
/// of uncertainty in the input scalar data, where the data at each grid point are
/// assumed to follow an independent Gaussian distribution. The mean and variance
/// fields define the distribution at each point. The filter outputs expected
/// isosurface vertex positions and a per-vertex crossing variance field.
///
/// This variant computes edge-crossing uncertainty using a closed-form
/// derivation (Hinkley/Marsaglia ratio formulation).
class VISKORES_FILTER_UNCERTAINTY_EXPORT ContourUncertainGaussianIndependentClosedForm
  : public viskores::filter::contour::AbstractContour
{
private:
  std::string CrossingVarianceName = "variance_edge_crossing";
  std::string ExpectedCrossingName = "expected_edge_crossing";

public:
  /// @brief Sets the mean field.
  /// Sets the name of the field containing the mean of the Gaussian distribution
  /// at each grid point.
  VISKORES_CONT void SetMeanField(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the variance field.
  /// Sets the name of the field containing the variance of the Gaussian
  /// distribution at each grid point.
  VISKORES_CONT void SetVarianceField(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the crossing variance output field.
  /// Sets the name of the output field storing the variance of the isosurface
  /// crossing position along each grid edge.
  VISKORES_CONT void SetCrossingVarianceName(const std::string& name)
  {
    this->CrossingVarianceName = name;
  }

  /// @brief Gets the crossing variance output field.
  /// Gets the name of the output field storing the variance of the isosurface
  /// crossing position along each grid edge.
  VISKORES_CONT const std::string& GetCrossingVarianceName() const
  {
    return this->CrossingVarianceName;
  }

  /// @brief Sets the expected crossing output field.
  /// Sets the name of the output field storing the expected isosurface crossing
  /// position along each grid edge.
  VISKORES_CONT void SetExpectedCrossingName(const std::string& name)
  {
    this->ExpectedCrossingName = name;
  }

  /// @brief Gets the expected crossing output field.
  /// Gets the name of the output field storing the expected isosurface crossing
  /// position along each grid edge.
  VISKORES_CONT const std::string& GetExpectedCrossingName() const
  {
    return this->ExpectedCrossingName;
  }

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace uncertainty
} // namespace filter
} // namespace viskores

#endif // viskores_filter_uncertainty_ContourUncertainGaussianIndependentClosedForm_h