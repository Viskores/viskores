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

#ifndef viskores_filter_uncertainty_worklet_gaussian_ContourUncertainGaussianIndependent_h
#define viskores_filter_uncertainty_worklet_gaussian_ContourUncertainGaussianIndependent_h

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/uncertainty/worklet/gaussian/GaussianDistribution.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace detail
{

/// @brief Compute crossing variance and expected crossing on each crossed edge.
///
/// For every grid edge crossed by the isosurface, evaluates the closed-form
/// Gaussian inverse linear interpolation uncertainty using the mean and
/// variance at the two edge endpoints. Independent data: covariance is zero.
struct ComputeEdgeVarianceWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn edgeIds,
                                FieldIn interpMean,
                                WholeArrayIn inputMeans,
                                WholeArrayIn inputVariance,
                                FieldOut outputVariance,
                                FieldOut expectedCrossing);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  template <typename T, typename PortalType>
  VISKORES_EXEC void operator()(const viskores::Id2& edgeIds,
                                T interpMean,
                                const PortalType& inputMeans,
                                const PortalType& inputVariance,
                                T& outputEdgeVariance,
                                viskores::Float64& outputExpectedCrossing) const
  {
    T mean0 = inputMeans.Get(edgeIds[0]);
    T mean1 = inputMeans.Get(edgeIds[1]);
    T variance0 = inputVariance.Get(edgeIds[0]);
    T variance1 = inputVariance.Get(edgeIds[1]);

    viskores::filter::uncertainty::GaussianDistribution gaussian;
    viskores::Float64 crossingProbability;
    viskores::Float64 crossingVariance;

    gaussian.GaussianAlphaPdf(mean0,
                              variance0,
                              mean1,
                              variance1,
                              0.0,
                              interpMean,
                              &outputExpectedCrossing,
                              &crossingProbability,
                              &crossingVariance);

    outputEdgeVariance = static_cast<T>(crossingVariance);
  }
};

/// @brief Invoke the edge variance worklet over the crossed edges.
template <typename VarianceArrayType, typename EdgeVarianceArrayType>
VISKORES_CONT void ComputeEdgeVariance(
  const VarianceArrayType& variance,
  const viskores::cont::UnknownArrayHandle& outputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& edgeIdsUnknown,
  EdgeVarianceArrayType& outputEdgeVariance,
  viskores::cont::ArrayHandle<viskores::Float64>& expectedCrossing)
{
  VarianceArrayType outputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(outputMeanArrayUnknown, outputMeanArray);
  VarianceArrayType inputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputMeanArrayUnknown, inputMeanArray);

  viskores::cont::ArrayHandle<viskores::Id2> edgeIds;
  edgeIdsUnknown.AsArrayHandle(edgeIds);

  viskores::cont::Invoker invoke;
  invoke(ComputeEdgeVarianceWorklet{},
         edgeIds,
         outputMeanArray,
         inputMeanArray,
         variance,
         outputEdgeVariance,
         expectedCrossing);
}

/// @brief Monte Carlo variant of @ref ComputeEdgeVarianceWorklet.
///
/// For every grid edge crossed by the isosurface, draws @c NumSamples joint
/// Gaussian samples (via the shared standard-normal portal) and accumulates
/// the inverse-linear-interpolation mean and variance using Welford's
/// algorithm. Independent data: covariance is zero.
struct ComputeEdgeVarianceMonteCarloWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn edgeIds,
                                FieldIn interpMean,
                                WholeArrayIn inputMeans,
                                WholeArrayIn inputVariance,
                                WholeArrayIn randomSamples,
                                FieldOut outputVariance,
                                FieldOut expectedCrossing);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  VISKORES_CONT explicit ComputeEdgeVarianceMonteCarloWorklet(viskores::Id numSamples)
    : NumSamples(numSamples)
  {
  }

  template <typename T, typename PortalType, typename RandomPortalType>
  VISKORES_EXEC void operator()(const viskores::Id2& edgeIds,
                                T interpMean,
                                const PortalType& inputMeans,
                                const PortalType& inputVariance,
                                const RandomPortalType& randomSamples,
                                T& outputEdgeVariance,
                                viskores::Float64& outputExpectedCrossing) const
  {
    T mean0 = inputMeans.Get(edgeIds[0]);
    T mean1 = inputMeans.Get(edgeIds[1]);
    T variance0 = inputVariance.Get(edgeIds[0]);
    T variance1 = inputVariance.Get(edgeIds[1]);

    viskores::filter::uncertainty::GaussianDistribution gaussian;
    viskores::Float64 crossingProbability;
    viskores::Float64 crossingVariance;

    gaussian.GaussianAlphaMonteCarlo(mean0,
                                     variance0,
                                     mean1,
                                     variance1,
                                     0.0,
                                     interpMean,
                                     randomSamples,
                                     this->NumSamples,
                                     &outputExpectedCrossing,
                                     &crossingProbability,
                                     &crossingVariance);

    outputEdgeVariance = static_cast<T>(crossingVariance);
  }

private:
  viskores::Id NumSamples;
};

/// @brief Invoke the Monte Carlo edge variance worklet over the crossed edges.
template <typename VarianceArrayType, typename EdgeVarianceArrayType, typename RandomArrayType>
VISKORES_CONT void ComputeEdgeVarianceMonteCarlo(
  const VarianceArrayType& variance,
  const viskores::cont::UnknownArrayHandle& outputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& edgeIdsUnknown,
  const RandomArrayType& randomSamples,
  viskores::Id numSamples,
  EdgeVarianceArrayType& outputEdgeVariance,
  viskores::cont::ArrayHandle<viskores::Float64>& expectedCrossing)
{
  VarianceArrayType outputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(outputMeanArrayUnknown, outputMeanArray);
  VarianceArrayType inputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputMeanArrayUnknown, inputMeanArray);

  viskores::cont::ArrayHandle<viskores::Id2> edgeIds;
  edgeIdsUnknown.AsArrayHandle(edgeIds);

  viskores::cont::Invoker invoke;
  invoke(ComputeEdgeVarianceMonteCarloWorklet{ numSamples },
         edgeIds,
         outputMeanArray,
         inputMeanArray,
         variance,
         randomSamples,
         outputEdgeVariance,
         expectedCrossing);
}

} // namespace detail
} // namespace worklet
} // namespace viskores

#endif // viskores_filter_uncertainty_worklet_gaussian_ContourUncertainGaussianIndependent_h
