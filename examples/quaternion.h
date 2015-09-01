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
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
	Quaternion() { x = y = z = 0.0; w = 1.0; }
	Quaternion(double x, double y, double z, double w) : x(x), y(y), z(z), w(w) {};
	void set(double ax, double ay, double az, double aw) { x = ax; y = ay; z = az; w = aw; }
	void normalize()
	{
	    float norm = sqrt(x*x + y*y + z*z + w*w);
	    if (norm > 0.00001) { x /= norm;  y /= norm;  z /= norm;  w /= norm; }
	}
	void mul(Quaternion q)
	{
	    float tx, ty, tz, tw;
	    tx = w*q.x + x*q.w + y*q.z - z*q.y;
	    ty = w*q.y + y*q.w + z*q.x - x*q.z;
	    tz = w*q.z + z*q.w + x*q.y - y*q.x;
	    tw = w*q.w - x*q.x - y*q.y - z*q.z;

	    x = tx; y = ty; z = tz; w = tw;
	}
	void setEulerAngles(float pitch, float yaw, float roll)
	{
	    w = cos(pitch/2.0)*cos(yaw/2.0)*cos(roll/2.0) - sin(pitch/2.0)*sin(yaw/2.0)*sin(roll/2.0);
	    x = sin(pitch/2.0)*sin(yaw/2.0)*cos(roll/2.0) + cos(pitch/2.0)*cos(yaw/2.0)*sin(roll/2.0);
	    y = sin(pitch/2.0)*cos(yaw/2.0)*cos(roll/2.0) + cos(pitch/2.0)*sin(yaw/2.0)*sin(roll/2.0);
	    z = cos(pitch/2.0)*sin(yaw/2.0)*cos(roll/2.0) - sin(pitch/2.0)*cos(yaw/2.0)*sin(roll/2.0);

	    normalize();
	}

	void getRotMat(float* m) const
	{
	    for (int i=0; i<16; i++) m[i] = 0.0; m[15] = 1.0;
	    m[0]  = 1 - 2*y*y - 2*z*z;  m[1]  = 2*x*y - 2*z*w;      m[2]  = 2*x*z + 2*y*w;
	    m[4]  = 2*x*y + 2*z*w;      m[5]  = 1 - 2*x*x - 2*z*z;  m[6]  = 2*y*z - 2*x*w;
	    m[8]  = 2*x*z - 2*y*w;      m[9]  = 2*y*z + 2*x*w;      m[10] = 1 - 2*x*x - 2*y*y;
	}

	double x,y,z,w;
};


#endif /* QUATERNION_H_ */
