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
  MultiVariateMonteCarlo(const viskores::Range& minAxis,
                         const viskores::Range& maxAxis,
                         const viskores::Id numSamples)
    : inputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , inputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
    , NumSamples(numSamples){};

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const MinX& ensembleMinX,
                                const MaxX& ensembleMaxX,
                                const MinY& ensembleMinY,
                                const MaxY& ensembleMaxY,
                                OutCellFieldType& probability) const
  {

    viskores::FloatDefault minX_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.first);

    viskores::FloatDefault minY_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.second);

    viskores::FloatDefault maxX_user = static_cast<viskores::FloatDefault>(this->inputTopRight.first);

    viskores::FloatDefault maxY_user = static_cast<viskores::FloatDefault>(this->inputTopRight.second);

    viskores::FloatDefault n1 = 0.0;
    viskores::FloatDefault n2 = 0.0;
    viskores::Id nonZeroCases = 0;
    viskores::FloatDefault mcProbability = 0.0;

    viskores::FloatDefault minX_dataset = static_cast<viskores::FloatDefault>(ensembleMinX);
    viskores::FloatDefault maxX_dataset = static_cast<viskores::FloatDefault>(ensembleMaxX);
    viskores::FloatDefault minY_dataset = static_cast<viskores::FloatDefault>(ensembleMinY);
    viskores::FloatDefault maxY_dataset = static_cast<viskores::FloatDefault>(ensembleMaxY);

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

    if ((minX_dataset == maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      if ((minX_dataset <= maxX_user) && (minX_dataset >= minX_user) &&
          (minY_dataset <= maxY_user) && (minY_dataset >= minY_user))
      {
        NonZeroCases = this->NumSamples;
      }
    }

    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      if ((minX_intersection < maxX_intersection) && (minY_dataset >= minY_user) &&
          (minY_dataset <= maxY_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          N1 = distX(rng);

          if ((N1 > minX_user) && (N1 < maxX_user))
          {
            NonZeroCases++;
          }
        }
      }
    }
    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
      if ((minY_intersection < maxY_intersection) && (minX_dataset >= minX_user) &&
          (minX_dataset <= maxX_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {

          N2 = distY(rng);

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
    std::uniform_real_distribution<viskores::FloatDefault> GenerateN1(minX_dataset, maxX_dataset);
    std::uniform_real_distribution<viskores::FloatDefault> GenerateN2(minY_dataset, maxY_dataset);

    if ((minX_dataset == maxX_dataset) && (minY_dataset == maxY_dataset))
    {

      if ((minX_dataset <= maxX_user) && (minX_dataset >= minX_user) &&
          (minY_dataset <= maxY_user) && (minY_dataset >= minY_user))
      {
        nonZeroCases = this->NumSamples;
      }
    }

    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      if ((minX_intersection < maxX_intersection) && (minY_dataset >= minY_user) &&
          (minY_dataset <= maxY_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          n1 = GenerateN1(gen);

          if ((n1 > minX_user) && (n1 < maxX_user))
          {
            nonZeroCases++;
          }
        }
      }
    }

    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
      if ((minY_intersection < maxY_intersection) && (minX_dataset >= minX_user) &&
          (minX_dataset <= maxX_user))
      {
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {

          n2 = GenerateN2(gen);

          if ((n2 > minY_user) && (n2 < maxY_user))
          {
            nonZeroCases++;
          }
        }
      }
    }
    else
    {
      for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
      {
        n1 = GenerateN1(gen);
        n2 = GenerateN2(gen);
        // Increase the case number when the data is located in user rectangle
        if ((n1 > minX_user) && (n1 < maxX_user) && (n2 > minY_user) && (n2 < maxY_user))
        {
          nonZeroCases++;
        }
      }
    }

#endif
    mcProbability = static_cast<viskores::FloatDefault>(nonZeroCases) /
      static_cast<viskores::FloatDefault>(this->NumSamples);
    probability = mcProbability;

    return;
  }

private:
  viskores::Pair<viskores::Float64, viskores::Float64> inputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> inputTopRight;
  viskores::Id NumSamples = 1;
};

class MultiVariateClosedForm : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateClosedForm(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : inputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , inputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>
  VISKORES_EXEC void operator()(const MinX& ensembleMinX,
                                const MaxX& ensembleMaxX,
                                const MinY& ensembleMinY,
                                const MaxY& ensembleMaxY,
                                OutCellFieldType& probability) const
  {

    viskores::FloatDefault minX_user = 0.0;
    minX_user = static_cast<viskores::FloatDefault>(inputBottomLeft.first);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(inputBottomLeft.second);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(inputTopRight.first);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(inputTopRight.second);

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

    minX_dataset = static_cast<viskores::FloatDefault>(ensembleMinX);
    maxX_dataset = static_cast<viskores::FloatDefault>(ensembleMaxX);
    minY_dataset = static_cast<viskores::FloatDefault>(ensembleMinY);
    maxY_dataset = static_cast<viskores::FloatDefault>(ensembleMaxY);

    minX_intersection = std::max(minX_user, minX_dataset);
    minY_intersection = std::max(minY_user, minY_dataset);
    maxX_intersection = std::min(maxX_user, maxX_dataset);
    maxY_intersection = std::min(maxY_user, maxY_dataset);

    if ((minX_dataset == maxX_dataset) && (minY_dataset == maxY_dataset))
    {
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
    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
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
    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
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
        IntersectionProbability = IntersectionArea / DataArea;
      }
    }
    probability = IntersectionProbability;
    return;
  }

private:
  viskores::Pair<viskores::Float64, viskores::Float64> inputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> inputTopRight;
};

class MultiVariateMean : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateMean(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : inputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , inputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const MinX& ensembleMinX,
                                const MaxX& ensembleMaxX,
                                const MinY& ensembleMinY,
                                const MaxY& ensembleMaxY,
                                OutCellFieldType& probability) const
  {

    viskores::FloatDefault minX_user = 0.0;
    minX_user = static_cast<viskores::FloatDefault>(inputBottomLeft.first);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(inputBottomLeft.second);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(inputTopRight.first);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(inputTopRight.second);

    viskores::FloatDefault minX_dataset = 0.0;
    viskores::FloatDefault minY_dataset = 0.0;
    viskores::FloatDefault maxX_dataset = 0.0;
    viskores::FloatDefault maxY_dataset = 0.0;

    minX_dataset = static_cast<viskores::FloatDefault>(ensembleMinX);
    maxX_dataset = static_cast<viskores::FloatDefault>(ensembleMaxX);
    minY_dataset = static_cast<viskores::FloatDefault>(ensembleMinY);
    maxY_dataset = static_cast<viskores::FloatDefault>(ensembleMaxY);

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
  viskores::Pair<viskores::Float64, viskores::Float64> inputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> inputTopRight;
};

class MultiVariateTruth : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateTruth(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : inputBottomLeft(
        viskores::Pair<viskores::Float64, viskores::Float64>(minAxis.Min, minAxis.Max))
    , inputTopRight(viskores::Pair<viskores::Float64, viskores::Float64>(maxAxis.Min, maxAxis.Max))
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename MinX, typename MaxX, typename MinY, typename MaxY, typename OutCellFieldType>

  VISKORES_EXEC void operator()(const MinX& ensembleMinX,
                                const MaxX& ensembleMaxX,
                                const MinY& ensembleMinY,
                                const MaxY& ensembleMaxY,
                                OutCellFieldType& probability) const
  {

    viskores::FloatDefault minX_user = 0.0;
    minX_user = static_cast<viskores::FloatDefault>(inputBottomLeft.first);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(inputBottomLeft.second);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(inputTopRight.first);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(inputTopRight.second);

    viskores::FloatDefault minX_dataset = 0.0;
    viskores::FloatDefault minY_dataset = 0.0;
    viskores::FloatDefault maxX_dataset = 0.0;
    viskores::FloatDefault maxY_dataset = 0.0;

    minX_dataset = static_cast<viskores::FloatDefault>(ensembleMinX);
    maxX_dataset = static_cast<viskores::FloatDefault>(ensembleMaxX);
    minY_dataset = static_cast<viskores::FloatDefault>(ensembleMinY);
    maxY_dataset = static_cast<viskores::FloatDefault>(ensembleMaxY);

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
  viskores::Pair<viskores::Float64, viskores::Float64> inputBottomLeft;
  viskores::Pair<viskores::Float64, viskores::Float64> inputTopRight;
};




}
}
}
#endif
