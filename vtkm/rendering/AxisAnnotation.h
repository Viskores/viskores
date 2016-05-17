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
#ifndef vtk_m_rendering_AxisAnnotation_h
#define vtk_m_rendering_AxisAnnotation_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm {
namespace rendering {

class AxisAnnotation
{
private:
protected:
  inline double ffix(double value)
  {
    int ivalue = (int)(value);
    double v = (value - ivalue);
    if (v > 0.9999)
    {
      ivalue++;
    }
    return (double) ivalue;
  }

  void CalculateTicks(double lower, double upper, bool minor,
                      std::vector<double> &positions,
                      std::vector<double> &proportions,
                      int modifyTickQuantity)
  {
    positions.clear();
    proportions.clear();

    double sortedRange[2];
    sortedRange[0] = lower < upper ? lower : upper;
    sortedRange[1] = lower > upper ? lower : upper;

    double range = sortedRange[1] - sortedRange[0];

    // Find the integral points.
    double pow10 = log10(range);

    // Build in numerical tolerance
    double eps = 10.0e-10;
    pow10 += eps;


    // ffix moves you in the wrong direction if pow10 is negative.
    if (pow10 < 0.)
    {
      pow10 = pow10 - 1.;
    }

    double fxt = pow(10., ffix(pow10));

    // Find the number of integral points in the interval.
    int numTicks = int(ffix(range/fxt) + 1);

    // We should get about major 10 ticks on a range that's near
    // the power of 10.  (e.g. range=1000).  If the range is small
    // enough we have less than 5 ticks (e.g. range=400), then
    // divide the step by 2, or if it's about 2 ticks (e.g. range=150)
    // or less, then divide the step by 5.  That gets us back to
    // about 10 major ticks.
    //
    // But we might want more or less.  To adjust this up by
    // approximately a factor of 2, instead of the default
    // 1/2/5 dividers, use 2/5/10, and to adjust it down by
    // about a factor of two, use .5/1/2 as the dividers.
    // (We constrain to 1s, 2s, and 5s, for the obvious reason
    // that only those values are factors of 10.....)
    double divs[5] = { 0.5, 1, 2, 5, 10 };
    int divindex = (numTicks >= 5) ? 1 : (numTicks >= 3 ? 2 : 3);
    divindex += modifyTickQuantity;

    double div = divs[divindex];

    // If there aren't enough major tick points in this decade, use the next
    // decade.
    double majorStep = fxt / div;
    double minorStep = (fxt/div) / 10.;

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
        positions[0] = lower;
        positions[1] = (lower+upper) / 2.;
        positions[2] = upper;
        proportions[0] = 0.0;
        proportions[1] = 0.5;
        proportions[2] = 1.0;
        return;
      }
    }

    // Figure out the first major and minor tick locations, relative to the
    // start of the axis.
    double majorStart, minorStart;
    if (sortedRange[0] < 0.)
    {
      majorStart = majorStep*(ffix(sortedRange[0]*(1./majorStep)));
      minorStart = minorStep*(ffix(sortedRange[0]*(1./minorStep)));
    }
    else
    {
      majorStart = majorStep*(ffix(sortedRange[0]*(1./majorStep) + .999));
      minorStart = minorStep*(ffix(sortedRange[0]*(1./minorStep) + .999));
    }

    // Create all of the minor ticks
    const int max_count_cutoff = 1000;
    numTicks = 0;
    double location = minor ? minorStart : majorStart;
    double step = minor ? minorStep : majorStep;
    while (location <= sortedRange[1] && numTicks < max_count_cutoff)
    {
      positions.push_back(location);
      proportions.push_back((location - sortedRange[0]) / range);
      numTicks++;
      location += step;
    }

    if (sortedRange[0] != lower)
    {
      // We must reverse all of the proportions.
      for (unsigned int j = 0 ; j < proportions.size(); j++)
      {
        proportions[j] = 1. - proportions[j];
      }
    }
  }
};


}} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation_h
