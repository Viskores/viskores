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
//  Spatial Correlation. The closed-form inverse linear interpolation
//  uncertainty uses Hinkley's derivation for the ratio of two correlated
//  Gaussian random variables. [D. Hinkley, "On the Ratio of Two Correlated
//  Normal Random Variables", 1969].

#ifndef viskores_filter_uncertainty_worklet_gaussian_GaussianDistribution_h
#define viskores_filter_uncertainty_worklet_gaussian_GaussianDistribution_h

#include <viskores/Math.h>

namespace viskores
{
namespace filter
{
namespace uncertainty
{

/// @brief Closed-form uncertainty of inverse linear interpolation for Gaussian data.
///
/// Provides the device-callable math used by the Gaussian contour uncertainty
/// filters. Given the mean, variance, and covariance of two Gaussian random
/// variables at the ends of a grid edge, it computes the expected position and
/// variance of the isosurface crossing along that edge.
class GaussianDistribution
{
public:
  /// @brief Compute the uncertainty of the isosurface crossing on a grid edge.
  ///
  /// @a muX, @a varX describe the Gaussian at one edge endpoint; @a muY, @a varY
  /// the other. @a covarXY is their covariance (zero for independent data).
  /// @a isovalue is the contour value. The expected crossing position, crossing
  /// probability, and crossing variance are returned through @a expt,
  /// @a crossProb, and @a var.
  VISKORES_EXEC void GaussianAlphaPdf(viskores::Float64 muX,
                                      viskores::Float64 varX,
                                      viskores::Float64 muY,
                                      viskores::Float64 varY,
                                      viskores::Float64 covarXY,
                                      viskores::Float64 isovalue,
                                      viskores::Float64* expt,
                                      viskores::Float64* crossProb,
                                      viskores::Float64* var) const
  {
    // Standard deviation of c - X.
    viskores::Float64 sigCX = viskores::Sqrt(varX);

    // Standard deviation of Y - X. Guard against a negative radicand (which
    // should be highly unlikely for a valid covariance) producing a NaN.
    viskores::Float64 sigYX;
    if (varX + varY - 2.0 * covarXY > 0.0)
    {
      sigYX = viskores::Sqrt(varX + varY - 2.0 * covarXY);
    }
    else
    {
      sigYX = viskores::Sqrt(0.00000001);
    }

    // Correlation between c - X and Y - X.
    viskores::Float64 pearsonCorrelation = (varX - covarXY) / (sigCX * sigYX);

    // Numerical roundoff under perfect correlation can push the ratio slightly
    // outside [-1, 1]; clamp it back.
    if (pearsonCorrelation > 1.0)
    {
      pearsonCorrelation = 0.99;
    }
    if (pearsonCorrelation < -1.0)
    {
      pearsonCorrelation = -0.99;
    }

    this->ComputeAlphaUncertainty(
      isovalue - muX, muY - muX, sigCX, sigYX, pearsonCorrelation, expt, crossProb, var);
  }

  /// @brief Expected value and variance of the inverse linear interpolation.
  ///
  /// Integrates the crossing-position density over [0, 1] (the valid range for
  /// marching cubes) to produce the expected crossing position and its variance.
  VISKORES_EXEC void ComputeAlphaUncertainty(viskores::Float64 muNumerator,
                                             viskores::Float64 muDenominator,
                                             viskores::Float64 sigNumerator,
                                             viskores::Float64 sigDenominator,
                                             viskores::Float64 rhoNumDenom,
                                             viskores::Float64* expt,
                                             viskores::Float64* crossProb,
                                             viskores::Float64* var) const
  {
    constexpr viskores::IdComponent numPoints = 100;
    viskores::Float64 alphaDensity[numPoints];
    viskores::Float64 alphaSum = 0.0;
    viskores::Float64 alphaExpected = 0.0;
    viskores::Float64 alphaVar = 0.0;

    // Sample the crossing-position density at equispaced locations in [0, 1].
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      viskores::Float64 alphaVal = static_cast<viskores::Float64>(pointIndex) * (1.0 / numPoints);
      viskores::Float64 density = this->HinkleyGaussianRatioCorrelated(
        muNumerator, muDenominator, sigNumerator, sigDenominator, rhoNumDenom, alphaVal);
      alphaDensity[pointIndex] = density;
      alphaSum += density;
    }

    // Expected crossing position, using the normalized density.
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      viskores::Float64 alphaVal = static_cast<viskores::Float64>(pointIndex) * (1.0 / numPoints);
      alphaExpected += (alphaDensity[pointIndex] / alphaSum) * alphaVal;
    }

    // Variance of the crossing position about the expected value.
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      viskores::Float64 alphaVal = static_cast<viskores::Float64>(pointIndex) * (1.0 / numPoints);
      alphaVar +=
        (alphaDensity[pointIndex] / alphaSum) * viskores::Pow(alphaVal - alphaExpected, 2.0);
    }

    *expt = alphaExpected;
    *crossProb = alphaSum;
    *var = alphaVar;
  }

  /// @brief Density of the ratio of two correlated Gaussian random variables.
  ///
  /// Transforms the correlated numerator and denominator into an equivalent
  /// uncorrelated pair, then defers to the independent ratio density.
  VISKORES_EXEC viskores::Float64 HinkleyGaussianRatioCorrelated(viskores::Float64 muNumerator,
                                                                 viskores::Float64 muDenominator,
                                                                 viskores::Float64 sigNumerator,
                                                                 viskores::Float64 sigDenominator,
                                                                 viskores::Float64 rhoNumDenom,
                                                                 viskores::Float64 alphaVal) const
  {
    viskores::Float64 muNumeratorDash =
      muNumerator - (rhoNumDenom * muDenominator * sigNumerator / sigDenominator);
    viskores::Float64 sigNumeratorDash =
      sigNumerator * viskores::Sqrt(1.0 - rhoNumDenom * rhoNumDenom);
    viskores::Float64 offset = rhoNumDenom * sigNumerator / sigDenominator;
    return this->HinkleyGaussianRatioIndependent(
      muNumeratorDash, muDenominator, sigNumeratorDash, sigDenominator, alphaVal - offset);
  }

  /// @brief Density of the ratio of two independent Gaussian random variables.
  ///
  /// Uses the Marsaglia and Hinkley derivations. A small epsilon guards against
  /// division by zero when a standard deviation is (near) zero.
  VISKORES_EXEC viskores::Float64 HinkleyGaussianRatioIndependent(viskores::Float64 muNumerator,
                                                                  viskores::Float64 muDenominator,
                                                                  viskores::Float64 sigNumerator,
                                                                  viskores::Float64 sigDenominator,
                                                                  viskores::Float64 alphaVal) const
  {
    if (sigNumerator < 0.0001)
    {
      sigNumerator = 0.0001;
    }
    if (sigDenominator < 0.0001)
    {
      sigDenominator = 0.0001;
    }

    viskores::Float64 az =
      viskores::Sqrt(viskores::Pow(alphaVal, 2.0) / viskores::Pow(sigNumerator, 2.0) +
                     1.0 / viskores::Pow(sigDenominator, 2.0));
    viskores::Float64 bz = muNumerator * alphaVal / viskores::Pow(sigNumerator, 2.0) +
      muDenominator / viskores::Pow(sigDenominator, 2.0);
    viskores::Float64 cz = viskores::Pow(muNumerator, 2.0) / viskores::Pow(sigNumerator, 2.0) +
      viskores::Pow(muDenominator, 2.0) / viskores::Pow(sigDenominator, 2.0);
    viskores::Float64 dz = viskores::Exp((viskores::Pow(bz, 2.0) - cz * viskores::Pow(az, 2.0)) /
                                         (2.0 * viskores::Pow(az, 2.0)));
    viskores::Float64 t1 = this->NormalCumulativeDistribution(bz / az, 0.0, 1.0);
    viskores::Float64 t2 = this->NormalCumulativeDistribution(-bz / az, 0.0, 1.0);

    return (bz * dz / viskores::Pow(az, 3.0)) *
      (1.0 / (viskores::Sqrt(2.0 * viskores::Pi()) * sigNumerator * sigDenominator)) * (t1 - t2) +
      1.0 / (viskores::Pow(az, 2.0) * viskores::Pi() * sigNumerator * sigDenominator) *
      viskores::Exp(-cz / 2.0);
  }

  /// @brief Cumulative distribution function of a Gaussian distribution.
  VISKORES_EXEC viskores::Float64 NormalCumulativeDistribution(viskores::Float64 t,
                                                               viskores::Float64 mu,
                                                               viskores::Float64 sig) const
  {
    return 0.5 * (1.0 + this->Erf((t - mu) / (viskores::Sqrt(2.0) * sig)));
  }

  /// @brief Monte Carlo estimate of the inverse linear interpolation uncertainty.
  ///
  /// Draws @a numSamples joint Gaussian samples by transforming pairs of
  /// standard-normal draws through a 2x2 closed-form eigen-decomposition of
  /// the covariance matrix, computes the inverse linear interpolation alpha
  /// for each sample, rejects samples with degenerate denominators or alphas
  /// outside [0, 1], and accumulates the mean and variance via Welford's
  /// running-stats algorithm. If no samples produce a valid alpha, falls back
  /// to the edge midpoint (expt = 0.5, var = 0).
  ///
  /// @a randomSamples must be a portal into an array of size at least
  /// `numSamples * 2`. Pairs `(randomSamples[2*i], randomSamples[2*i + 1])`
  /// are read as independent standard-normal draws.
  template <typename RandomPortalType>
  VISKORES_EXEC void GaussianAlphaMonteCarlo(viskores::Float64 muX,
                                             viskores::Float64 varX,
                                             viskores::Float64 muY,
                                             viskores::Float64 varY,
                                             viskores::Float64 covarXY,
                                             viskores::Float64 isovalue,
                                             const RandomPortalType& randomSamples,
                                             viskores::Id numSamples,
                                             viskores::Float64* expt,
                                             viskores::Float64* crossProb,
                                             viskores::Float64* var) const
  {
    // Closed-form 2x2 eigen-decomposition of [[varX, covarXY], [covarXY, varY]].
    // For a symmetric 2x2 matrix the eigenvalues and eigenvectors have analytic
    // forms; we clamp the discriminant and the eigenvalues at zero to absorb
    // floating-point roundoff for (near-)singular covariance matrices.
    viskores::Float64 trace = varX + varY;
    viskores::Float64 determinant = varX * varY - covarXY * covarXY;
    viskores::Float64 discriminant =
      viskores::Sqrt(viskores::Max(0.0, trace * trace - 4.0 * determinant));
    viskores::Float64 lambda0 = viskores::Max(0.0, 0.5 * (trace + discriminant));
    viskores::Float64 lambda1 = viskores::Max(0.0, 0.5 * (trace - discriminant));
    viskores::Float64 sqrtLambda0 = viskores::Sqrt(lambda0);
    viskores::Float64 sqrtLambda1 = viskores::Sqrt(lambda1);

    // Eigenvectors of [[a, b], [b, c]] for eigenvalue lambda are proportional
    // to (lambda - c, b). When b == 0 the matrix is already diagonal and the
    // standard basis is the eigenvector basis.
    viskores::Float64 v00, v10, v01, v11;
    if (viskores::Abs(covarXY) > 1e-12)
    {
      viskores::Float64 norm0 =
        viskores::Sqrt((lambda0 - varY) * (lambda0 - varY) + covarXY * covarXY);
      v00 = (lambda0 - varY) / norm0;
      v10 = covarXY / norm0;
      viskores::Float64 norm1 =
        viskores::Sqrt((lambda1 - varY) * (lambda1 - varY) + covarXY * covarXY);
      v01 = (lambda1 - varY) / norm1;
      v11 = covarXY / norm1;
    }
    else
    {
      // Diagonal covariance: the standard basis is the eigenvector basis, so use
      // varX and varY directly rather than the magnitude-sorted lambda0/lambda1.
      // Sorting breaks the dimension correspondence when varX < varY.
      lambda0 = varX;
      lambda1 = varY;
      sqrtLambda0 = viskores::Sqrt(viskores::Max(0.0, lambda0));
      sqrtLambda1 = viskores::Sqrt(viskores::Max(0.0, lambda1));
      v00 = 1.0;
      v10 = 0.0;
      v01 = 0.0;
      v11 = 1.0;
    }

    // Sampling transform A = V * diag(sqrt(lambda)). Columns of A scale the
    // standard-normal samples into the joint Gaussian.
    viskores::Float64 a00 = v00 * sqrtLambda0;
    viskores::Float64 a10 = v10 * sqrtLambda0;
    viskores::Float64 a01 = v01 * sqrtLambda1;
    viskores::Float64 a11 = v11 * sqrtLambda1;

    // Welford running stats over accepted samples.
    viskores::Id acceptedCount = 0;
    viskores::Float64 runningMean = 0.0;
    viskores::Float64 runningM2 = 0.0;

    for (viskores::Id sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
      viskores::Float64 z0 = randomSamples.Get(2 * sampleIndex);
      viskores::Float64 z1 = randomSamples.Get(2 * sampleIndex + 1);
      viskores::Float64 x0 = muX + a00 * z0 + a01 * z1;
      viskores::Float64 x1 = muY + a10 * z0 + a11 * z1;

      viskores::Float64 denominator = x1 - x0;
      if (viskores::Abs(denominator) < 0.001)
      {
        continue;
      }
      viskores::Float64 alpha = (isovalue - x0) / denominator;
      if (alpha < 0.0 || alpha > 1.0)
      {
        continue;
      }

      ++acceptedCount;
      viskores::Float64 delta = alpha - runningMean;
      runningMean += delta / static_cast<viskores::Float64>(acceptedCount);
      viskores::Float64 delta2 = alpha - runningMean;
      runningM2 += delta * delta2;
    }

    if (acceptedCount > 0)
    {
      *expt = runningMean;
      *var = runningM2 / static_cast<viskores::Float64>(acceptedCount);
    }
    else
    {
      // No samples produced a valid alpha. Fall back to a deterministic
      // edge-midpoint output with zero spread.
      *expt = 0.5;
      *var = 0.0;
    }
    *crossProb =
      static_cast<viskores::Float64>(acceptedCount) / static_cast<viskores::Float64>(numSamples);
  }

private:
  /// @brief Error function via the Abramowitz & Stegun 7.1.26 approximation.
  ///
  /// Device-portable replacement for std::erf (which is not reliably callable
  /// across all device adapters). Maximum absolute error is about 1.5e-7.
  VISKORES_EXEC viskores::Float64 Erf(viskores::Float64 x) const
  {
    viskores::Float64 sign = 1.0;
    if (x < 0.0)
    {
      sign = -1.0;
    }
    x = viskores::Abs(x);

    viskores::Float64 t = 1.0 / (1.0 + 0.3275911 * x);
    viskores::Float64 y = 1.0 -
      (((((1.061405429 * t - 1.453152027) * t) + 1.421413741) * t - 0.284496736) * t +
       0.254829592) *
        t * viskores::Exp(-x * x);

    return sign * y;
  }
};

} // namespace uncertainty
} // namespace filter
} // namespace viskores

#endif // viskores_filter_uncertainty_worklet_gaussian_GaussianDistribution_h
