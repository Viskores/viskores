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

#ifndef viskores_filter_uncertainty_FiberUncertainUniform_inner_h
#define viskores_filter_uncertainty_FiberUncertainUniform_inner_h
#include <iostream>
#include <utility>
#include <vector>
#include <viskores/filter/uncertainty/worklet/FiberUncertainUniform.h>
#include <viskores/worklet/WorkletMapField.h>


#if defined(VISKORES_CUDA) || defined(VISKORES_KOKKOS_HIP)
#include <thrust/device_vector.h>
#include <thrust/random/linear_congruential_engine.h>
#include <thrust/random/uniform_real_distribution.h>
#else
#include <random>
#endif

namespace viskores
{
namespace worklet
{
namespace detail
{
class MultiVariateMonteCarlo : public viskores::worklet::WorkletMapField
{
public:
  // Worklet Input
  // Fiber(const std::vector<std::pair<double, double>>& minAxis,
  //      const std::vector<std::pair<double, double>>& maxAxis)
  //  : InputBottomLeft(minAxis), InputTopRight(maxAxis){};
  MultiVariateMonteCarlo(const viskores::Range& minAxis,
                         const viskores::Range& maxAxis,
                         const viskores::Id numSamples)
    : InputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , InputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
    , NumSamples(numSamples){};

  // Input and Output Parameters
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Template
  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>
  // Operator
  VISKORES_EXEC void operator()(const MinX& EnsembleMinX,
                                const MaxX& EnsembleMaxX,
                                const MinY& EnsembleMinY,
                                const MaxY& EnsembleMaxY,
                                OutCellFieldType& probability) const
  {
    // User defined rectangle (trait)
    viskores::FloatDefault minX_user = static_cast<viskores::FloatDefault>(InputBottomLeft.first);

    viskores::FloatDefault minY_user = static_cast<viskores::FloatDefault>(InputBottomLeft.second);

    viskores::FloatDefault maxX_user = static_cast<viskores::FloatDefault>(InputTopRight.first);

    viskores::FloatDefault maxY_user = static_cast<viskores::FloatDefault>(InputTopRight.second);



    viskores::FloatDefault N1 = 0.0;
    viskores::FloatDefault N2 = 0.0;
    viskores::Id NonZeroCases = 0;
    viskores::FloatDefault MCProbability = 0.0;

    // Data rectangle
    viskores::FloatDefault minX_dataset = static_cast<viskores::FloatDefault>(EnsembleMinX);
    viskores::FloatDefault maxX_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxX);
    viskores::FloatDefault minY_dataset = static_cast<viskores::FloatDefault>(EnsembleMinY);
    viskores::FloatDefault maxY_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxY);


    viskores::FloatDefault minX_intersection = std::max(minX_user, minX_dataset);
    viskores::FloatDefault minY_intersection = std::max(minY_user, minY_dataset);
    viskores::FloatDefault maxX_intersection = std::min(maxX_user, maxX_dataset);
    viskores::FloatDefault maxY_intersection = std::min(maxY_user, maxY_dataset);


// Monte Carlo
// Trait Coordinates (minX_user,minY_user) & (maxX_user,maxY_user)
#if defined(VISKORES_CUDA) || defined(VISKORES_KOKKOS_HIP)
    thrust::minstd_rand rng;
    thrust::uniform_real_distribution<viskores::FloatDefault> distX(minX_dataset, maxX_dataset);
    thrust::uniform_real_distribution<viskores::FloatDefault> distY(minY_dataset, maxY_dataset);

    // Point
    if ((minX_dataset == maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      // check if point is inside a trait
      if ((minX_dataset <= maxX_user) && (minX_dataset >= minX_user) &&
          (minY_dataset <= maxY_user) && (minY_dataset >= minY_user))
      {
        NonZeroCases = this->NumSamples;
      }
    }

    // Line
    // Data as horizontal line
    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      // Check if line intersects a trait. Might need <= check here
      if ((minX_intersection < maxX_intersection) && (minY_dataset >= minY_user) &&
          (minY_dataset <= maxY_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          N1 = distX(rng);
          // Increase the case number when the data is located in user rectangle
          if ((N1 > minX_user) && (N1 < maxX_user))
          {
            NonZeroCases++;
          }
        }
      }
    }

    // Data as a vertical line
    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
      // Check if line intersects a trait. Might need <= check here
      if ((minY_intersection < maxY_intersection) && (minX_dataset >= minX_user) &&
          (minX_dataset <= maxX_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {

          N2 = distY(rng);
          // Increase the case number when the data is located in user rectangle
          if ((N2 > minY_user) && (N2 < maxY_user))
          {
            NonZeroCases++;
          }
        }
      }
    }

    else
    {
      for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
      {
        N1 = distX(rng);
        N2 = distY(rng);
        if ((N1 > minX_user) && (N1 < maxX_user) && (N2 > minY_user) && (N2 < maxY_user))
        {
          NonZeroCases++;
        }
      }
    }

#else
    std::random_device rd;
    std::mt19937 gen(rd());
    // Generate samples from data rectangle
    std::uniform_real_distribution<viskores::FloatDefault> GenerateN1(minX_dataset, maxX_dataset);
    std::uniform_real_distribution<viskores::FloatDefault> GenerateN2(minY_dataset, maxY_dataset);

    // Point
    if ((minX_dataset == maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      // Check if point is inside a trait
      if ((minX_dataset <= maxX_user) && (minX_dataset >= minX_user) &&
          (minY_dataset <= maxY_user) && (minY_dataset >= minY_user))
      {
        NonZeroCases = this->NumSamples;
      }
    }

    // Line
    // Data as horizontal line
    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      // Check if line intersects a trait. Might need <= check here
      if ((minX_intersection < maxX_intersection) && (minY_dataset >= minY_user) &&
          (minY_dataset <= maxY_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          N1 = GenerateN1(gen);
          // Increase the case number when the data is located in user rectangle
          if ((N1 > minX_user) && (N1 < maxX_user))
          {
            NonZeroCases++;
          }
        }
      }
    }

    // Data as a vertical line
    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
      // Check if line intersects a trait. Might need <= check here
      if ((minY_intersection < maxY_intersection) && (minX_dataset >= minX_user) &&
          (minX_dataset <= maxX_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {

          N2 = GenerateN2(gen);
          // Increase the case number when the data is located in user rectangle
          if ((N2 > minY_user) && (N2 < maxY_user))
          {
            NonZeroCases++;
          }
        }
      }
    }

    // Rectangle
    else
    {
      for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
      {
        N1 = GenerateN1(gen);
        N2 = GenerateN2(gen);
        // Increase the case number when the data is located in user rectangle
        if ((N1 > minX_user) && (N1 < maxX_user) && (N2 > minY_user) && (N2 < maxY_user))
        {
          NonZeroCases++;
        }
      }
    }

#endif
    MCProbability = static_cast<viskores::FloatDefault>(NonZeroCases) /
      static_cast<viskores::FloatDefault>(this->NumSamples);
    probability = MCProbability;

    return;
  }

private:
  viskores::Pair<viskores::Float64, viskores::Float64> InputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> InputTopRight;
  viskores::Id NumSamples = 1;
};

class MultiVariateClosedForm : public viskores::worklet::WorkletMapField
{
public:
  // Worklet Input
  // Fiber(const std::vector<std::pair<double, double>>& minAxis,
  //      const std::vector<std::pair<double, double>>& maxAxis)
  //  : InputBottomLeft(minAxis), InputTopRight(maxAxis){};
  MultiVariateClosedForm(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : InputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , InputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
  {
  }

  // Input and Output Parameters
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  // Template
  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>
  // Operator
  VISKORES_EXEC void operator()(const MinX& EnsembleMinX,
                                const MaxX& EnsembleMaxX,
                                const MinY& EnsembleMinY,
                                const MaxY& EnsembleMaxY,
                                OutCellFieldType& probability) const
  {
    // User defined rectangle (trait)
    viskores::FloatDefault minX_user = 0.0;
    minX_user = static_cast<viskores::FloatDefault>(InputBottomLeft.first);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(InputBottomLeft.second);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(InputTopRight.first);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(InputTopRight.second);

    viskores::FloatDefault minX_intersection = 0.0;
    viskores::FloatDefault maxX_intersection = 0.0;
    viskores::FloatDefault minY_intersection = 0.0;
    viskores::FloatDefault maxY_intersection = 0.0;

    viskores::FloatDefault minX_dataset = 0.0;
    viskores::FloatDefault minY_dataset = 0.0;
    viskores::FloatDefault maxX_dataset = 0.0;
    viskores::FloatDefault maxY_dataset = 0.0;

    viskores::FloatDefault IntersectionArea = 0.0;
    viskores::FloatDefault IntersectionProbability = 0.0;
    viskores::FloatDefault IntersectionHeight = 0.0;
    viskores::FloatDefault IntersectionWidth = 0.0;

    // Data rectangle
    minX_dataset = static_cast<viskores::FloatDefault>(EnsembleMinX);
    maxX_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxX);
    minY_dataset = static_cast<viskores::FloatDefault>(EnsembleMinY);
    maxY_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxY);

    // Calculating data and trait intersection limits. For intersection to take place, mixX <= maxX and minY <= maxY condition must satisfy.
    minX_intersection = std::max(minX_user, minX_dataset);
    minY_intersection = std::max(minY_user, minY_dataset);
    maxX_intersection = std::min(maxX_user, maxX_dataset);
    maxY_intersection = std::min(maxY_user, maxY_dataset);

    // Check if data is a point, line, or a rectangle

    // Point
    if ((minX_dataset == maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      // check if point is inside a trait
      if ((minX_dataset <= maxX_user) && (minX_dataset >= minX_user) &&
          (minY_dataset <= maxY_user) && (minY_dataset >= minY_user))
      {
        IntersectionProbability = 1.0;
      }
      else
      {
        IntersectionProbability = 0.0;
      }
    }

    // Line
    // Data as horizontal line
    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      // Check if line intersects a trait. Might need <= check here
      if ((minX_intersection < maxX_intersection) && (minY_dataset >= minY_user) &&
          (minY_dataset <= maxY_user))
      {
        IntersectionProbability =
          (maxX_intersection - minX_intersection) / (maxX_dataset - minX_dataset);
      }
      else
      {
        IntersectionProbability = 0.0;
      }
    }

    // Data as a vertical line
    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
      // Check if line intersects a trait. Might need <= check here
      if ((minY_intersection < maxY_intersection) && (minX_dataset >= minX_user) &&
          (minX_dataset <= maxX_user))
      {
        IntersectionProbability =
          (maxY_intersection - minY_intersection) / (maxY_dataset - minY_dataset);
      }
      else
      {
        IntersectionProbability = 0.0;
      }
    }

    // Rectangle
    else
    {
      IntersectionHeight = maxY_intersection - minY_intersection;
      IntersectionWidth = maxX_intersection - minX_intersection;

      viskores::FloatDefault DataArea =
        (maxX_dataset - minX_dataset) * (maxY_dataset - minY_dataset);

      if ((IntersectionHeight > 0) && (IntersectionWidth > 0) &&
          (minX_intersection < maxX_intersection) && (minY_intersection < maxY_intersection))
      {
        IntersectionArea = IntersectionHeight * IntersectionWidth;
        // The portion of trait
        IntersectionProbability = IntersectionArea / DataArea;
      }
    }
    probability = IntersectionProbability;
    return;
  }

private:
  viskores::Pair<viskores::Float64, viskores::Float64> InputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> InputTopRight;
};

class MultiVariateMean : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateMean(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : InputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , InputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
  {
  }

  // Input and Output Parameters
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const MinX& EnsembleMinX,
                                const MaxX& EnsembleMaxX,
                                const MinY& EnsembleMinY,
                                const MaxY& EnsembleMaxY,
                                OutCellFieldType& probability) const
  {

    // User defined rectangle (trait)
    viskores::FloatDefault minX_user = 0.0;
    minX_user = static_cast<viskores::FloatDefault>(InputBottomLeft.first);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(InputBottomLeft.second);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(InputTopRight.first);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(InputTopRight.second);

    viskores::FloatDefault minX_dataset = 0.0;
    viskores::FloatDefault minY_dataset = 0.0;
    viskores::FloatDefault maxX_dataset = 0.0;
    viskores::FloatDefault maxY_dataset = 0.0;

    // Data rectangle
    minX_dataset = static_cast<viskores::FloatDefault>(EnsembleMinX);
    maxX_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxX);
    minY_dataset = static_cast<viskores::FloatDefault>(EnsembleMinY);
    maxY_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxY);

    viskores::FloatDefault Xmean = 0.0;
    viskores::FloatDefault Ymean = 0.0;
    Xmean = (minX_dataset + maxX_dataset) / 2;
    Ymean = (minY_dataset + maxY_dataset) / 2;

    if ((Xmean <= maxX_user) && (Xmean >= minX_user) && (Ymean <= maxY_user) &&
        (Ymean >= minY_user))
    {
      probability = 1.0;
      return;
    }
    else
    {
      probability = 0.0;
      return;
    }
  }

private:
  viskores::Pair<viskores::Float64, viskores::Float64> InputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> InputTopRight;
};

class MultiVariateTruth : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateTruth(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : InputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , InputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
  {
  }

  // Input and Output Parameters
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const MinX& EnsembleMinX,
                                const MaxX& EnsembleMaxX,
                                const MinY& EnsembleMinY,
                                const MaxY& EnsembleMaxY,
                                OutCellFieldType& probability) const
  {

    // User defined rectangle (trait)
    viskores::FloatDefault minX_user = 0.0;
    minX_user = static_cast<viskores::FloatDefault>(InputBottomLeft.first);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(InputBottomLeft.second);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(InputTopRight.first);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(InputTopRight.second);

    viskores::FloatDefault minX_dataset = 0.0;
    viskores::FloatDefault minY_dataset = 0.0;
    viskores::FloatDefault maxX_dataset = 0.0;
    viskores::FloatDefault maxY_dataset = 0.0;

    // Data rectangle
    minX_dataset = static_cast<viskores::FloatDefault>(EnsembleMinX);
    maxX_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxX);
    minY_dataset = static_cast<viskores::FloatDefault>(EnsembleMinY);
    maxY_dataset = static_cast<viskores::FloatDefault>(EnsembleMaxY);

    if ((maxX_dataset <= maxX_user) && (minX_dataset >= minX_user) && (maxY_dataset <= maxY_user) &&
        (minY_dataset >= minY_user))
    {
      probability = 1.0;
      return;
    }
    else
    {
      probability = 0.0;
      return;
    }
  }

private:
  viskores::Pair<viskores::Float64, viskores::Float64> InputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> InputTopRight;
};




}
}
}
#endif
