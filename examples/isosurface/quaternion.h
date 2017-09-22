//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

/*
 * quaternion.h
 *
 *  Created on: Oct 10, 2014
 *      Author: csewell
 */

#ifndef QUATERNION_H_
#define QUATERNION_H_

#include <math.h>
#include <stdlib.h>

class Quaternion
{
public:
  Quaternion()
  {
    x = y = z = 0.0;
    w = 1.0;
  }
  Quaternion(double ax, double ay, double az, double aw)
    : x(ax)
    , y(ay)
    , z(az)
    , w(aw){};
  void set(double ax, double ay, double az, double aw)
  {
    x = ax;
    y = ay;
    z = az;
    w = aw;
  }
  void normalize()
  {
    float norm = static_cast<float>(sqrt(x * x + y * y + z * z + w * w));
    if (norm > 0.00001)
    {
      x /= norm;
      y /= norm;
      z /= norm;
      w /= norm;
    }
  }
  void mul(Quaternion q)
  {
    double tx, ty, tz, tw;
    tx = w * q.x + x * q.w + y * q.z - z * q.y;
    ty = w * q.y + y * q.w + z * q.x - x * q.z;
    tz = w * q.z + z * q.w + x * q.y - y * q.x;
    tw = w * q.w - x * q.x - y * q.y - z * q.z;

    x = tx;
    y = ty;
    z = tz;
    w = tw;
  }
  void setEulerAngles(float pitch, float yaw, float roll)
  {
    w = cos(pitch / 2.0) * cos(yaw / 2.0) * cos(roll / 2.0) -
      sin(pitch / 2.0) * sin(yaw / 2.0) * sin(roll / 2.0);
    x = sin(pitch / 2.0) * sin(yaw / 2.0) * cos(roll / 2.0) +
      cos(pitch / 2.0) * cos(yaw / 2.0) * sin(roll / 2.0);
    y = sin(pitch / 2.0) * cos(yaw / 2.0) * cos(roll / 2.0) +
      cos(pitch / 2.0) * sin(yaw / 2.0) * sin(roll / 2.0);
    z = cos(pitch / 2.0) * sin(yaw / 2.0) * cos(roll / 2.0) -
      sin(pitch / 2.0) * cos(yaw / 2.0) * sin(roll / 2.0);

    normalize();
  }

  void getRotMat(float* m) const
  {
    for (int i = 0; i < 16; i++)
    {
      m[i] = 0.0;
    }

    m[0] = static_cast<float>(1.0 - 2.0 * y * y - 2.0 * z * z);
    m[1] = static_cast<float>(2.0 * x * y - 2.0 * z * w);
    m[2] = static_cast<float>(2.0 * x * z + 2.0 * y * w);

    m[4] = static_cast<float>(2.0 * x * y + 2.0 * z * w);
    m[5] = static_cast<float>(1.0 - 2.0 * x * x - 2.0 * z * z);
    m[6] = static_cast<float>(2.0 * y * z - 2.0 * x * w);

    m[8] = static_cast<float>(2.0 * x * z - 2.0 * y * w);
    m[9] = static_cast<float>(2.0 * y * z + 2.0 * x * w);
    m[10] = static_cast<float>(1.0 - 2.0 * x * x - 2.0 * y * y);

    m[15] = 1.0;
  }

  double x, y, z, w;
};

#endif /* QUATERNION_H_ */
