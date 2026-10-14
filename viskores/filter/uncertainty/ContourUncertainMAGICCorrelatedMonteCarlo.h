//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_uncertainty_ContourUncertainMAGICCorrelatedMonteCarlo_h
#define viskores_filter_uncertainty_ContourUncertainMAGICCorrelatedMonteCarlo_h

#include <viskores/filter/contour/AbstractContour.h>
#include <viskores/filter/uncertainty/viskores_filter_uncertainty_export.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{

/// @brief Visualize isosurface uncertainty for spatially correlated Gaussian data.
///
/// This filter implements the Monte Carlo variant of the correlated MAGIC
/// algorithm. It requires point fields containing the Gaussian mean, variance,
/// and covariance along each axis of a structured 3D grid.
class VISKORES_FILTER_UNCERTAINTY_EXPORT ContourUncertainMAGICCorrelatedMonteCarlo
  : public viskores::filter::contour::AbstractContour
{
private:
  std::string CrossingVarianceName = "variance_edge_crossing";
  std::string ExpectedCrossingName = "expected_edge_crossing";
  viskores::Id NumberOfSamples = 4000;

public:
  /// @brief Sets the point field containing the Gaussian means.
  VISKORES_CONT void SetMeanField(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the point field containing the Gaussian variances.
  VISKORES_CONT void SetVarianceField(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the point field containing covariance along the X axis.
  VISKORES_CONT void SetRhoXField(const std::string& fieldName)
  {
    this->SetActiveField(2, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the point field containing covariance along the Y axis.
  VISKORES_CONT void SetRhoYField(const std::string& fieldName)
  {
    this->SetActiveField(3, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the point field containing covariance along the Z axis.
  VISKORES_CONT void SetRhoZField(const std::string& fieldName)
  {
    this->SetActiveField(4, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the name of the output crossing-variance field.
  VISKORES_CONT void SetCrossingVarianceName(const std::string& name)
  {
    this->CrossingVarianceName = name;
  }

  /// @brief Gets the name of the output crossing-variance field.
  VISKORES_CONT const std::string& GetCrossingVarianceName() const
  {
    return this->CrossingVarianceName;
  }

  /// @brief Sets the name of the output expected-crossing field.
  VISKORES_CONT void SetExpectedCrossingName(const std::string& name)
  {
    this->ExpectedCrossingName = name;
  }

  /// @brief Gets the name of the output expected-crossing field.
  VISKORES_CONT const std::string& GetExpectedCrossingName() const
  {
    return this->ExpectedCrossingName;
  }

  /// @brief Sets the number of Monte Carlo samples per edge.
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

#endif // viskores_filter_uncertainty_ContourUncertainMAGICCorrelatedMonteCarlo_h
