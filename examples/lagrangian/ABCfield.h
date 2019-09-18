//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
class ABCfield
{

public:
  void calculateVelocity(double* location, double t, double* velocity)
  {
    double ep = 0.25;
    double period = 1.0;

    double sinval = ep * vtkm::Sin(period * t);

    velocity[0] = vtkm::Sin(location[2] + sinval) + vtkm::Cos(location[1] + sinval);
    velocity[1] = vtkm::Sin(location[0] + sinval) + vtkm::Cos(location[2] + sinval);
    velocity[2] = vtkm::Sin(location[1] + sinval) + vtkm::Cos(location[0] + sinval);
  }
};
