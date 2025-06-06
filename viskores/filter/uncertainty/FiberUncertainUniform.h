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

//  This code is based on the algorithm presented in the following papers:
//  Hari, G., Joshi, N., Wang, Z., Gong, Q., Pugmire, D., Moreland, K.,
//  Johnson, C. R., Klasky, S., Podhorszki, N., and Athawale, T. M.
//  (2024). FunM^2C: A Filter for Uncertainty Visualization of MultiVariate Data
//  on Multi-Core Devices. Oak Ridge National Laboratory (ORNL),
//  Oak Ridge, TN (United States).

#ifndef viskores_filter_uncertainty_FiberUncertainUniform_h
#define viskores_filter_uncertainty_FiberUncertainUniform_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/uncertainty/viskores_filter_uncertainty_export.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{
/// @brief Visualize fiber surface uncertainty for uniform distributed data.
///
/// This filter computes the positional uncertainty of fiber surfaces as a
/// function of uncertainty in bivariate input data, where the data associated with each variable are assumed to
/// be uniformly distributed and sampled on a regular grid. The uniform
/// distribution range is given through the input datasets via the minimum
/// and maximum ensemble X and Ys. Given the uniform distribution range, the computed
/// fiber surface uncertainty corresponds to uncertainty in bivariate cases in
/// the fiber uncertainty visualization algorithm.
///
class VISKORES_FILTER_UNCERTAINTY_EXPORT FiberUncertainUniform : public viskores::filter::Filter
{
public:
  /// @brief Constructor
  VISKORES_CONT FiberUncertainUniform() = default;

  /// @brief Sets minimum axis.
  /// Sets minimum value of uniform distribution at each grid point.
  VISKORES_CONT void SetMinAxis(
    const viskores::Pair<viskores::FloatDefault, viskores::FloatDefault>& minCoordinate)
  {
    this->minAxis = minCoordinate;
  }

  /// @brief Sets maximum axis.
  /// Sets minimum value of uniform distrubition at each grid point.
  VISKORES_CONT void SetMaxAxis(
    const viskores::Pair<viskores::FloatDefault, viskores::FloatDefault>& maxCoordinate)
  {
    this->maxAxis = maxCoordinate;
  }

  /// @brief Sets minimum X.
  /// Sets minimum value for trait rectangle's bottom-left corner.
  VISKORES_CONT void SetMinX(const std::string& fieldName)
  {
    this->SetActiveField(0, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets maxmimum Y.
  /// Sets maxmimum value for trait rectangle's bottom right corner.
  VISKORES_CONT void SetMaxX(const std::string& fieldName)
  {
    this->SetActiveField(1, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets minimum Y.
  /// Sets minimum value for trait rectangle's top left corner.
  VISKORES_CONT void SetMinY(const std::string& fieldName)
  {
    this->SetActiveField(2, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets maxmimum Y.
  /// Sets maxmimum value for trait rectangle's top right corner.
  VISKORES_CONT void SetMaxY(const std::string& fieldName)
  {
    this->SetActiveField(3, fieldName, viskores::cont::Field::Association::Points);
  }

  /// @brief Sets number of samples.
  /// Sets how many samples will be used when computing uncertainty via Monte Carlo.
  VISKORES_CONT void SetNumSamples(const viskores::Id& numSamples)
  {
    this->NumSamples = numSamples;
  }

  /// @brief Sets the approach for computing uncertainty.
  ///  Sets the formula for the corresponding filter that was selected.
  VISKORES_CONT void SetApproach(const std::string& approach)
  {
    this->Approach = approach;
  }

private:
  viskores::Pair<viskores::FloatDefault, viskores::FloatDefault> minAxis;
  viskores::Pair<viskores::FloatDefault, viskores::FloatDefault> maxAxis;
  std::string Approach = "ClosedForm"; // MonteCarlo, ClosedForm, Mean, Truth
  viskores::Id NumSamples = 5000;

  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& input) override;
};

} 
} 
} 

#endif
