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

#ifndef viskores_filter_uncertainty_worklet_gaussian_ContourUncertainGaussianCorrelated_h
#define viskores_filter_uncertainty_worklet_gaussian_ContourUncertainGaussianCorrelated_h

#include <viskores/Math.h>
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
/// Gaussian inverse linear interpolation uncertainty using the mean, variance,
/// and the appropriate per-axis covariance at the two edge endpoints. The
/// covariance field used depends on which physical axis the edge runs along.
struct ComputeEdgeVarianceCorrelatedWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn edgeIds,
                                FieldIn interpMean,
                                WholeArrayIn inputMeans,
                                WholeArrayIn inputVariance,
                                WholeArrayIn inputRhoX,
                                WholeArrayIn inputRhoY,
                                WholeArrayIn inputRhoZ,
                                FieldOut outputVariance,
                                FieldOut expectedCrossing);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  using InputDomain = _1;

  VISKORES_CONT explicit ComputeEdgeVarianceCorrelatedWorklet(viskores::Id3 resolution)
    : Resolution(resolution)
  {
  }

  template <typename T, typename PortalType>
  VISKORES_EXEC void operator()(const viskores::Id2& edgeIds,
                                T interpMean,
                                const PortalType& inputMeans,
                                const PortalType& inputVariance,
                                const PortalType& inputRhoX,
                                const PortalType& inputRhoY,
                                const PortalType& inputRhoZ,
                                T& outputEdgeVariance,
                                viskores::Float64& outputExpectedCrossing) const
  {
    T mean0 = inputMeans.Get(edgeIds[0]);
    T mean1 = inputMeans.Get(edgeIds[1]);
    T variance0 = inputVariance.Get(edgeIds[0]);
    T variance1 = inputVariance.Get(edgeIds[1]);

    // Decode each flat point index into its grid coordinates under standard
    // VTK structured-points storage (X varies fastest, Z slowest).
    const viskores::Id dimX = this->Resolution[0];
    const viskores::Id dimXY = this->Resolution[1] * this->Resolution[0];

    viskores::Id xIndex0 = edgeIds[0] % dimX;
    viskores::Id yIndex0 = (edgeIds[0] % dimXY) / dimX;
    viskores::Id zIndex0 = edgeIds[0] / dimXY;

    viskores::Id xIndex1 = edgeIds[1] % dimX;
    viskores::Id yIndex1 = (edgeIds[1] % dimXY) / dimX;
    viskores::Id zIndex1 = edgeIds[1] / dimXY;

    // Pick the covariance field matching the axis along which the edge runs.
    // The covariance is stored at the lower endpoint of the edge by convention.
    viskores::Float64 covariance = 0.0;
    if (viskores::Abs(xIndex1 - xIndex0) == 1)
    {
      if (xIndex0 < xIndex1)
      {
        covariance = inputRhoX.Get(edgeIds[0]);
      }
      else
      {
        covariance = inputRhoX.Get(edgeIds[1]);
      }
    }
    else if (viskores::Abs(yIndex1 - yIndex0) == 1)
    {
      if (yIndex0 < yIndex1)
      {
        covariance = inputRhoY.Get(edgeIds[0]);
      }
      else
      {
        covariance = inputRhoY.Get(edgeIds[1]);
      }
    }
    else if (viskores::Abs(zIndex1 - zIndex0) == 1)
    {
      if (zIndex0 < zIndex1)
      {
        covariance = inputRhoZ.Get(edgeIds[0]);
      }
      else
      {
        covariance = inputRhoZ.Get(edgeIds[1]);
      }
    }

    viskores::filter::uncertainty::GaussianDistribution gaussian;
    viskores::Float64 crossingProbability;
    viskores::Float64 crossingVariance;

    gaussian.GaussianAlphaPdf(mean0,
                              variance0,
                              mean1,
                              variance1,
                              covariance,
                              interpMean,
                              &outputExpectedCrossing,
                              &crossingProbability,
                              &crossingVariance);

    outputEdgeVariance = static_cast<T>(crossingVariance);
  }

private:
  viskores::Id3 Resolution;
};

/// @brief Invoke the correlated edge variance worklet over the crossed edges.
template <typename VarianceArrayType, typename EdgeVarianceArrayType>
VISKORES_CONT void ComputeEdgeVarianceCorrelated(
  const VarianceArrayType& variance,
  const viskores::cont::UnknownArrayHandle& inputRhoXArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputRhoYArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputRhoZArrayUnknown,
  const viskores::cont::UnknownArrayHandle& outputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& edgeIdsUnknown,
  viskores::Id3 resolution,
  EdgeVarianceArrayType& outputEdgeVariance,
  viskores::cont::ArrayHandle<viskores::Float64>& expectedCrossing)
{
  VarianceArrayType outputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(outputMeanArrayUnknown, outputMeanArray);
  VarianceArrayType inputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputMeanArrayUnknown, inputMeanArray);
  VarianceArrayType inputRhoXArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputRhoXArrayUnknown, inputRhoXArray);
  VarianceArrayType inputRhoYArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputRhoYArrayUnknown, inputRhoYArray);
  VarianceArrayType inputRhoZArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputRhoZArrayUnknown, inputRhoZArray);

  viskores::cont::ArrayHandle<viskores::Id2> edgeIds;
  edgeIdsUnknown.AsArrayHandle(edgeIds);

  viskores::cont::Invoker invoke;
  invoke(ComputeEdgeVarianceCorrelatedWorklet{ resolution },
         edgeIds,
         outputMeanArray,
         inputMeanArray,
         variance,
         inputRhoXArray,
         inputRhoYArray,
         inputRhoZArray,
         outputEdgeVariance,
         expectedCrossing);
}

/// @brief Monte Carlo variant of @ref ComputeEdgeVarianceCorrelatedWorklet.
///
/// Identical to the closed-form variant in axis selection and covariance lookup,
/// but delegates to GaussianDistribution::GaussianAlphaMonteCarlo rather than
/// GaussianAlphaPdf to accumulate the crossing statistics.
struct ComputeEdgeVarianceCorrelatedMonteCarloWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn edgeIds,
                                FieldIn interpMean,
                                WholeArrayIn inputMeans,
                                WholeArrayIn inputVariance,
                                WholeArrayIn inputRhoX,
                                WholeArrayIn inputRhoY,
                                WholeArrayIn inputRhoZ,
                                WholeArrayIn randomSamples,
                                FieldOut outputVariance,
                                FieldOut expectedCrossing);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10);
  using InputDomain = _1;

  VISKORES_CONT explicit ComputeEdgeVarianceCorrelatedMonteCarloWorklet(viskores::Id3 resolution,
                                                                        viskores::Id numSamples)
    : Resolution(resolution)
    , NumSamples(numSamples)
  {
  }

  template <typename T, typename PortalType, typename RandomPortalType>
  VISKORES_EXEC void operator()(const viskores::Id2& edgeIds,
                                T interpMean,
                                const PortalType& inputMeans,
                                const PortalType& inputVariance,
                                const PortalType& inputRhoX,
                                const PortalType& inputRhoY,
                                const PortalType& inputRhoZ,
                                const RandomPortalType& randomSamples,
                                T& outputEdgeVariance,
                                viskores::Float64& outputExpectedCrossing) const
  {
    T mean0 = inputMeans.Get(edgeIds[0]);
    T mean1 = inputMeans.Get(edgeIds[1]);
    T variance0 = inputVariance.Get(edgeIds[0]);
    T variance1 = inputVariance.Get(edgeIds[1]);

    const viskores::Id dimX = this->Resolution[0];
    const viskores::Id dimXY = this->Resolution[1] * this->Resolution[0];

    viskores::Id xIndex0 = edgeIds[0] % dimX;
    viskores::Id yIndex0 = (edgeIds[0] % dimXY) / dimX;
    viskores::Id zIndex0 = edgeIds[0] / dimXY;

    viskores::Id xIndex1 = edgeIds[1] % dimX;
    viskores::Id yIndex1 = (edgeIds[1] % dimXY) / dimX;
    viskores::Id zIndex1 = edgeIds[1] / dimXY;

    viskores::Float64 covariance = 0.0;
    if (viskores::Abs(xIndex1 - xIndex0) == 1)
    {
      if (xIndex0 < xIndex1)
      {
        covariance = inputRhoX.Get(edgeIds[0]);
      }
      else
      {
        covariance = inputRhoX.Get(edgeIds[1]);
      }
    }
    else if (viskores::Abs(yIndex1 - yIndex0) == 1)
    {
      if (yIndex0 < yIndex1)
      {
        covariance = inputRhoY.Get(edgeIds[0]);
      }
      else
      {
        covariance = inputRhoY.Get(edgeIds[1]);
      }
    }
    else if (viskores::Abs(zIndex1 - zIndex0) == 1)
    {
      if (zIndex0 < zIndex1)
      {
        covariance = inputRhoZ.Get(edgeIds[0]);
      }
      else
      {
        covariance = inputRhoZ.Get(edgeIds[1]);
      }
    }

    viskores::filter::uncertainty::GaussianDistribution gaussian;
    viskores::Float64 crossingProbability;
    viskores::Float64 crossingVariance;

    gaussian.GaussianAlphaMonteCarlo(mean0,
                                     variance0,
                                     mean1,
                                     variance1,
                                     covariance,
                                     interpMean,
                                     randomSamples,
                                     this->NumSamples,
                                     &outputExpectedCrossing,
                                     &crossingProbability,
                                     &crossingVariance);

    outputEdgeVariance = static_cast<T>(crossingVariance);
  }

private:
  viskores::Id3 Resolution;
  viskores::Id NumSamples;
};

/// @brief Invoke the Monte Carlo correlated edge variance worklet over the crossed edges.
template <typename VarianceArrayType, typename EdgeVarianceArrayType, typename RandomArrayType>
VISKORES_CONT void ComputeEdgeVarianceCorrelatedMonteCarlo(
  const VarianceArrayType& variance,
  const viskores::cont::UnknownArrayHandle& inputRhoXArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputRhoYArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputRhoZArrayUnknown,
  const viskores::cont::UnknownArrayHandle& outputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& inputMeanArrayUnknown,
  const viskores::cont::UnknownArrayHandle& edgeIdsUnknown,
  const RandomArrayType& randomSamples,
  viskores::Id numSamples,
  viskores::Id3 resolution,
  EdgeVarianceArrayType& outputEdgeVariance,
  viskores::cont::ArrayHandle<viskores::Float64>& expectedCrossing)
{
  VarianceArrayType outputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(outputMeanArrayUnknown, outputMeanArray);
  VarianceArrayType inputMeanArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputMeanArrayUnknown, inputMeanArray);
  VarianceArrayType inputRhoXArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputRhoXArrayUnknown, inputRhoXArray);
  VarianceArrayType inputRhoYArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputRhoYArrayUnknown, inputRhoYArray);
  VarianceArrayType inputRhoZArray;
  viskores::cont::ArrayCopyShallowIfPossible(inputRhoZArrayUnknown, inputRhoZArray);

  viskores::cont::ArrayHandle<viskores::Id2> edgeIds;
  edgeIdsUnknown.AsArrayHandle(edgeIds);

  viskores::cont::Invoker invoke;
  invoke(ComputeEdgeVarianceCorrelatedMonteCarloWorklet{ resolution, numSamples },
         edgeIds,
         outputMeanArray,
         inputMeanArray,
         variance,
         inputRhoXArray,
         inputRhoYArray,
         inputRhoZArray,
         randomSamples,
         outputEdgeVariance,
         expectedCrossing);
}

} // namespace detail
} // namespace worklet
} // namespace viskores

#endif // viskores_filter_uncertainty_worklet_gaussian_ContourUncertainGaussianCorrelated_h
