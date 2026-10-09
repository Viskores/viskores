//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_uncertainty_ContourUncertainMAGICIndependentClosedForm_h
#define viskores_filter_uncertainty_ContourUncertainMAGICIndependentClosedForm_h

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
/// This filter implements the closed-form variant of the MAGIC algorithm. The
/// input point fields contain the mean and variance of an independent Gaussian
/// distribution. The output contains the expected edge-crossing positions and
/// their variances.
class VISKORES_FILTER_UNCERTAINTY_EXPORT ContourUncertainMAGICIndependentClosedForm
  : public viskores::filter::contour::AbstractContour
{
private:
  std::string CrossingVarianceName = "variance_edge_crossing";
  std::string ExpectedCrossingName = "expected_edge_crossing";

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

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace uncertainty
} // namespace filter
} // namespace viskores

#endif // viskores_filter_uncertainty_ContourUncertainMAGICIndependentClosedForm_h
