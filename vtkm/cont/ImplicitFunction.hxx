//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/VectorAnalysis.h>

namespace vtkm {
namespace cont {

//============================================================================
VTKM_EXEC_CONT
inline FloatDefault
Box::Value(const vtkm::Vec<FloatDefault, 3> &x) const
{
  FloatDefault minDistance = vtkm::NegativeInfinity32();
  FloatDefault diff, t, dist;
  FloatDefault distance = FloatDefault(0.0);
  vtkm::IdComponent inside = 1;

  for (vtkm::IdComponent d = 0; d < 3; d++)
  {
    diff = this->MaxPoint[d] - this->MinPoint[d];
    if (diff != FloatDefault(0.0))
    {
      t = (x[d] - this->MinPoint[d]) / diff;
      // Outside before the box
      if (t < FloatDefault(0.0))
      {
        inside = 0;
        dist = this->MinPoint[d] - x[d];
      }
      // Outside after the box
      else if (t > FloatDefault(1.0))
      {
        inside = 0;
        dist = x[d] - this->MaxPoint[d];
      }
      else
      {
        // Inside the box in lower half
        if (t <= FloatDefault(0.5))
        {
          dist = MinPoint[d] - x[d];
        }
        // Inside the box in upper half
        else
        {
          dist = x[d] - MaxPoint[d];
        }
        if (dist > minDistance)
        {
          minDistance = dist;
        }
      }
    }
    else
    {
      dist = vtkm::Abs(x[d] - MinPoint[d]);
      if (dist > FloatDefault(0.0))
      {
        inside = 0;
      }
    }
    if (dist > FloatDefault(0.0))
    {
      distance += dist*dist;
    }
  }

  distance = vtkm::Sqrt(distance);
  if (inside)
  {
    return minDistance;
  }
  else
  {
    return distance;
  }
}

//============================================================================
VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3>
Box::Gradient(const vtkm::Vec<FloatDefault, 3> &x) const
{
  vtkm::IdComponent minAxis = 0;
  FloatDefault dist = 0.0;
  FloatDefault minDist = vtkm::Infinity32();
  vtkm::Vec<vtkm::IdComponent,3> location;
  vtkm::Vec<FloatDefault,3> normal;
  vtkm::Vec<FloatDefault,3> inside(FloatDefault(0), FloatDefault(0), FloatDefault(0));
  vtkm::Vec<FloatDefault,3> outside(FloatDefault(0), FloatDefault(0), FloatDefault(0));
  vtkm::Vec<FloatDefault,3> center((this->MaxPoint[0] + this->MinPoint[0]) * FloatDefault(0.5),
                                   (this->MaxPoint[1] + this->MinPoint[1]) * FloatDefault(0.5),
                                   (this->MaxPoint[2] + this->MinPoint[2]) * FloatDefault(0.5));

  // Compute the location of the point with respect to the box
  // Point will lie in one of 27 separate regions around or within the box
  // Gradient vector is computed differently in each of the regions.
  for (vtkm::IdComponent d = 0; d < 3; d++)
  {
    if (x[d] < this->MinPoint[d])
    {
      // Outside the box low end
      location[d] = 0;
      outside[d] = -1.0;
    }
    else if (x[d] > this->MaxPoint[d])
    {
      // Outside the box high end
      location[d] = 2;
      outside[d] = 1.0;
    }
    else
    {
      location[d] = 1;
      if (x[d] <= center[d])
      {
        // Inside the box low end
        dist = x[d] - this->MinPoint[d];
        inside[d] = -1.0;
      }
      else
      {
        // Inside the box high end
        dist = this->MaxPoint[d] - x[d];
        inside[d] = 1.0;
      }
      if (dist < minDist) // dist is negative
      {
        minDist = dist;
        minAxis = d;
      }
    }
  }

  vtkm::Id indx = location[0] + 3*location[1] + 9*location[2];
  switch (indx)
  {
    // verts - gradient points away from center point
    case 0: case 2: case 6: case 8: case 18: case 20: case 24: case 26:
      for (vtkm::IdComponent d = 0; d < 3; d++)
      {
        normal[d] = x[d] - center[d];
      }
      vtkm::Normalize(normal);
      break;

    // edges - gradient points out from axis of cube
    case 1: case 3: case 5: case 7:
    case 9: case 11: case 15: case 17:
    case 19: case 21: case 23: case 25:
      for (vtkm::IdComponent d = 0; d < 3; d++)
      {
        if (outside[d] != 0.0)
        {
          normal[d] = x[d] - center[d];
        }
        else
        {
          normal[d] = 0.0;
        }
      }
      vtkm::Normalize(normal);
      break;

    // faces - gradient points perpendicular to face
    case 4: case 10: case 12: case 14: case 16: case 22:
      for (vtkm::IdComponent d = 0; d < 3; d++)
      {
        normal[d] = outside[d];
      }
      break;

    // interior - gradient is perpendicular to closest face
    case 13:
      normal[0] = normal[1] = normal[2] = 0.0;
      normal[minAxis] = inside[minAxis];
      break;
    default:
      VTKM_ASSERT(false);
      break;
  }
  return normal;
}


}
} // vtkm::cont
