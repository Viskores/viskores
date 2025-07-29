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
    : inputBottomLeft(minAxis.Min, minAxis.Max)
    , inputTopRight(maxAxis.Min, maxAxis.Max)
    , NumSamples(numSamples){};

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldOut, WholeArrayIn);

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  using InputDomain = _1;

  template <typename MinX,
            typename MaxX, 
            typename MinY, 
            typename MaxY, 
            typename OutCellFieldType, 
            typename RandomPortalType>

  VISKORES_EXEC void operator()(const MinX& ensembleMinX,
                                const MaxX& ensembleMaxX,
                                const MinY& ensembleMinY,
                                const MaxY& ensembleMaxY,
                                OutCellFieldType& probability,
                                const RandomPortalType& randomPortal) const
  {
    viskores::FloatDefault minX_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Min);

    viskores::FloatDefault minY_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Max);

    viskores::FloatDefault maxX_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Min);

    viskores::FloatDefault maxY_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Max);

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
        viskores::FloatDefault rangeX = maxX_dataset - minX_dataset;
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          viskores::FloatDefault r1 = randomPortal.Get(i);
          viskores::FloatDefault n1 = minX_dataset + r1 * rangeX;

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
        viskores::FloatDefault rangeY = maxY_dataset - minY_dataset;
        for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
        {
          viskores::FloatDefault r2 = randomPortal.Get(i + this->NumSamples);
          viskores::FloatDefault n2 = minY_dataset + r2 * rangeY;

          if ((n2 > minY_user) && (n2 < maxY_user))
          {
            nonZeroCases++;
          }
        }
      }
    }
    else
    {
      viskores::FloatDefault rangeX = maxX_dataset - minX_dataset;
      viskores::FloatDefault rangeY = maxY_dataset - minY_dataset;
      for (viskores::IdComponent i = 0; i < this->NumSamples; i++)
      {
        viskores::FloatDefault r1 = randomPortal.Get(i);
        viskores::FloatDefault r2 = randomPortal.Get(i + this->NumSamples);
        viskores::FloatDefault n1 = minX_dataset + r1 * rangeX;
        viskores::FloatDefault n2 = minY_dataset + r2 * rangeY;

        if ((n1 > minX_user) && (n1 < maxX_user) && (n2 > minY_user) && (n2 < maxY_user))
        {
          nonZeroCases++;
        }
      }
    }

    mcProbability = static_cast<viskores::FloatDefault>(nonZeroCases) /
     static_cast<viskores::FloatDefault>(this->NumSamples);
    probability = mcProbability;

    return;
  }

private:
  viskores::Range inputBottomLeft;
  viskores::Range inputTopRight;
  viskores::Id NumSamples = 1;
};

class MultiVariateClosedForm : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateClosedForm(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : inputBottomLeft(minAxis.Min, minAxis.Max)
    , inputTopRight(maxAxis.Min, maxAxis.Max)
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
    minX_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Min);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Max);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Min);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Max);

    viskores::FloatDefault minX_intersection = 0.0;
    viskores::FloatDefault maxX_intersection = 0.0;
    viskores::FloatDefault minY_intersection = 0.0;
    viskores::FloatDefault maxY_intersection = 0.0;

    viskores::FloatDefault minX_dataset = 0.0;
    viskores::FloatDefault minY_dataset = 0.0;
    viskores::FloatDefault maxX_dataset = 0.0;
    viskores::FloatDefault maxY_dataset = 0.0;

    viskores::FloatDefault intersectionArea = 0.0;
    viskores::FloatDefault intersectionProbability = 0.0;
    viskores::FloatDefault intersectionHeight = 0.0;
    viskores::FloatDefault intersectionWidth = 0.0;

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
        intersectionProbability = 1.0;
      }
      else
      {
        intersectionProbability = 0.0;
      }
    }
    else if ((minX_dataset < maxX_dataset) && (minY_dataset == maxY_dataset))
    {
      if ((minX_intersection < maxX_intersection) && (minY_dataset >= minY_user) &&
          (minY_dataset <= maxY_user))
      {
        intersectionProbability =
          (maxX_intersection - minX_intersection) / (maxX_dataset - minX_dataset);
      }
      else
      {
        intersectionProbability = 0.0;
      }
    }
    else if ((minX_dataset == maxX_dataset) && (minY_dataset < maxY_dataset))
    {
      if ((minY_intersection < maxY_intersection) && (minX_dataset >= minX_user) &&
          (minX_dataset <= maxX_user))
      {
        intersectionProbability =
          (maxY_intersection - minY_intersection) / (maxY_dataset - minY_dataset);
      }
      else
      {
        intersectionProbability = 0.0;
      }
    }
    else
    {
      intersectionHeight = maxY_intersection - minY_intersection;
      intersectionWidth = maxX_intersection - minX_intersection;

      viskores::FloatDefault DataArea =
        (maxX_dataset - minX_dataset) * (maxY_dataset - minY_dataset);

      if ((intersectionHeight > 0) && (intersectionWidth > 0) &&
          (minX_intersection < maxX_intersection) && (minY_intersection < maxY_intersection))
      {
        intersectionArea = intersectionHeight * intersectionWidth;
        intersectionProbability = intersectionArea / DataArea;
      }
    }
    probability = intersectionProbability;
    return;
  }

private:
  viskores::Range inputBottomLeft;
  viskores::Range inputTopRight;
};

class MultiVariateMean : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateMean(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : inputBottomLeft(minAxis.Min, minAxis.Max)
    , inputTopRight(maxAxis.Min, maxAxis.Max)
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
    minX_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Min);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Max);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Min);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Max);

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
  viskores::Range inputBottomLeft;
  viskores::Range inputTopRight;
};

class MultiVariateTruth : public viskores::worklet::WorkletMapField
{
public:
  MultiVariateTruth(const viskores::Range& minAxis, const viskores::Range& maxAxis)
    : inputBottomLeft(minAxis.Min, minAxis.Max)
    , inputTopRight(maxAxis.Min, maxAxis.Max)
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
    minX_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Min);
    viskores::FloatDefault minY_user = 0.0;
    minY_user = static_cast<viskores::FloatDefault>(this->inputBottomLeft.Max);
    viskores::FloatDefault maxX_user = 0.0;
    maxX_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Min);
    viskores::FloatDefault maxY_user = 0.0;
    maxY_user = static_cast<viskores::FloatDefault>(this->inputTopRight.Max);

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
  viskores::Range inputBottomLeft;
  viskores::Range inputTopRight;
};




}
}
}
#endif
