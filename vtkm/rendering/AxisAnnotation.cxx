//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/AxisAnnotation.h>

#include <vtkm/cont/ErrorBadType.h>

namespace vtkm
{
namespace rendering
{

namespace
{

inline vtkm::Float64 ffix(vtkm::Float64 value)
{
  int ivalue = (int)(value);
  vtkm::Float64 v = (value - ivalue);
  if (v > 0.9999)
  {
    ivalue++;
  }
  return static_cast<vtkm::Float64>(ivalue);
}

} // anonymous namespace

void AxisAnnotation::CalculateTicks(const vtkm::Range& range,
                                    bool minor,
                                    std::vector<vtkm::Float64>& positions,
                                    std::vector<vtkm::Float64>& proportions,
                                    int modifyTickQuantity) const
{
  positions.clear();
  proportions.clear();

  if (!range.IsNonEmpty())
  {
    return;
  }

  vtkm::Float64 length = range.Length();

  // Find the integral points.
  vtkm::Float64 pow10 = log10(length);

  // Build in numerical tolerance
  vtkm::Float64 eps = 10.0e-10;
  pow10 += eps;

  // ffix moves you in the wrong direction if pow10 is negative.
  if (pow10 < 0.)
  {
    pow10 = pow10 - 1.;
  }

  vtkm::Float64 fxt = pow(10., ffix(pow10));

  // Find the number of integral points in the interval.
  int numTicks = int(ffix(length / fxt) + 1);

  // We should get about major 10 ticks on a length that's near
  // the power of 10.  (e.g. length=1000).  If the length is small
  // enough we have less than 5 ticks (e.g. length=400), then
  // divide the step by 2, or if it's about 2 ticks (e.g. length=150)
  // or less, then divide the step by 5.  That gets us back to
  // about 10 major ticks.
  //
  // But we might want more or less.  To adjust this up by
  // approximately a factor of 2, instead of the default
  // 1/2/5 dividers, use 2/5/10, and to adjust it down by
  // about a factor of two, use .5/1/2 as the dividers.
  // (We constrain to 1s, 2s, and 5s, for the obvious reason
  // that only those values are factors of 10.....)
  vtkm::Float64 divs[5] = { 0.5, 1, 2, 5, 10 };
  int divindex = (numTicks >= 5) ? 1 : (numTicks >= 3 ? 2 : 3);
  divindex += modifyTickQuantity;

  vtkm::Float64 div = divs[divindex];

  // If there aren't enough major tick points in this decade, use the next
  // decade.
  vtkm::Float64 majorStep = fxt / div;
  vtkm::Float64 minorStep = (fxt / div) / 10.;

  // When we get too close, we lose the tickmarks. Run some special case code.
  if (numTicks <= 1)
  {
    if (minor)
    {
      // no minor ticks
      return;
    }
    else
    {
      positions.resize(3);
      proportions.resize(3);
      positions[0] = range.Min;
      positions[1] = range.Center();
      positions[2] = range.Max;
      proportions[0] = 0.0;
      proportions[1] = 0.5;
      proportions[2] = 1.0;
      return;
    }
  }

  // Figure out the first major and minor tick locations, relative to the
  // start of the axis.
  vtkm::Float64 majorStart, minorStart;
  if (range.Min < 0.)
  {
    majorStart = majorStep * (ffix(range.Min * (1. / majorStep)));
    minorStart = minorStep * (ffix(range.Min * (1. / minorStep)));
  }
  else
  {
    majorStart = majorStep * (ffix(range.Min * (1. / majorStep) + .999));
    minorStart = minorStep * (ffix(range.Min * (1. / minorStep) + .999));
  }

  // Create all of the minor ticks
  const int max_count_cutoff = 1000;
  numTicks = 0;
  vtkm::Float64 location = minor ? minorStart : majorStart;
  vtkm::Float64 step = minor ? minorStep : majorStep;
  while (location <= range.Max && numTicks < max_count_cutoff)
  {
    positions.push_back(location);
    proportions.push_back((location - range.Min) / length);
    numTicks++;
    location += step;
  }
}

AxisAnnotation::AxisAnnotation()
{
}

AxisAnnotation::~AxisAnnotation()
{
}
}
} // namespace vtkm::rendering
