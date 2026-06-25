//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_uncertainty_ContourUncertainGaussianIndependent_h
#define viskores_filter_uncertainty_ContourUncertainGaussianIndependent_h

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
class VISKORES_FILTER_UNCERTAINTY_EXPORT ContourUncertainGaussianIndependent
  : public viskores::filter::contour::AbstractContour
{
public:
  /// @brief Algorithm used to compute the isosurface uncertainty.
  enum struct ApproachEnum
  {
    ClosedForm,
    MonteCarlo
  };

private:
  std::string CrossingVarianceName = "variance_edge_crossing";
  std::string ExpectedCrossingName = "expected_edge_crossing";
  ApproachEnum Approach = ApproachEnum::ClosedForm;
  viskores::Id NumberOfSamples = 4000;

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

  /// @brief Sets the algorithm used to compute the isosurface uncertainty.
  /// `ClosedForm` (the default) uses the Hinkley/Marsaglia derivation.
  /// `MonteCarlo` samples the joint Gaussian distribution and accumulates
  /// statistics over the crossing positions; the sample count is controlled by
  /// @ref SetNumberOfSamples.
  VISKORES_CONT void SetApproach(ApproachEnum approach) { this->Approach = approach; }

  /// @brief Gets the algorithm used to compute the isosurface uncertainty.
  VISKORES_CONT ApproachEnum GetApproach() const { return this->Approach; }

  /// @brief Sets the number of Monte Carlo samples per edge.
  /// Only used when @ref SetApproach selects `MonteCarlo`.
  VISKORES_CONT void SetNumberOfSamples(viskores::Id numSamples)
  {
    this->NumberOfSamples = numSamples;
  }

  /// @brief Gets the number of Monte Carlo samples per edge.
  VISKORES_CONT viskores::Id GetNumberOfSamples() const { return this->NumberOfSamples; }

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace uncertainty
} // namespace filter
} // namespace viskores

#endif // viskores_filter_uncertainty_ContourUncertainGaussianIndependent_h