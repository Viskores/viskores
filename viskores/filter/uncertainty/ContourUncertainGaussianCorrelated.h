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

//  This code is based on the MAGIC algorithm:
//  Athawale, T., Moreland, K., Pugmire, D., Johnson, C., Rosen, P.,
//  Norman, M., Georgiadou, A., Entezari, A. (2025). MAGIC: Marching Cubes
//  Isosurface Uncertainty Visualization for Gaussian Uncertain Data with
//  Spatial Correlation.

#ifndef viskores_filter_uncertainty_ContourUncertainGaussianCorrelated_h
#define viskores_filter_uncertainty_ContourUncertainGaussianCorrelated_h

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
/// Extends the independent Gaussian filter by accounting for spatial correlation
/// between neighboring grid points. In addition to the mean and pointwise
/// variance fields, this filter accepts three per-axis edge covariance fields
/// (`RhoX`, `RhoY`, `RhoZ`) that encode the correlation between adjacent points
/// along each physical axis. The input must be a structured 3D grid. The filter
/// outputs the expected isosurface vertex positions and a per-vertex crossing
/// variance field.
///
class VISKORES_FILTER_UNCERTAINTY_EXPORT ContourUncertainGaussianCorrelated
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
  /// Sets the name of the field containing the pointwise variance of the
  /// Gaussian distribution at each grid point.
  VISKORES_CONT void SetVarianceField(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the covariance field along the X axis.
  /// Sets the name of the field containing the covariance between adjacent
  /// grid points along the physical X axis (the fastest-varying storage axis
  /// under standard VTK structured-points layout).
  VISKORES_CONT void SetRhoXField(const std::string& fieldName)
  {
    this->SetActiveField(2, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the covariance field along the Y axis.
  /// Sets the name of the field containing the covariance between adjacent
  /// grid points along the physical Y axis.
  VISKORES_CONT void SetRhoYField(const std::string& fieldName)
  {
    this->SetActiveField(3, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets the covariance field along the Z axis.
  /// Sets the name of the field containing the covariance between adjacent
  /// grid points along the physical Z axis (the slowest-varying storage axis
  /// under standard VTK structured-points layout).
  VISKORES_CONT void SetRhoZField(const std::string& fieldName)
  {
    this->SetActiveField(4, fieldName, viskores::cont::Field::Association::Points);
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

#endif // viskores_filter_uncertainty_ContourUncertainGaussianCorrelated_h
