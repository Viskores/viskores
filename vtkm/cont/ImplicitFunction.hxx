//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <algorithm>
#include <limits>

namespace vtkm
{
namespace cont
{

//============================================================================
inline Box::Box()
  : MinPoint(vtkm::Vec<FloatDefault, 3>(FloatDefault(0)))
  , MaxPoint(vtkm::Vec<FloatDefault, 3>(FloatDefault(1)))
{
}

inline Box::Box(vtkm::Vec<FloatDefault, 3> minPoint, vtkm::Vec<FloatDefault, 3> maxPoint)
  : MinPoint(minPoint)
  , MaxPoint(maxPoint)
{
}

inline Box::Box(FloatDefault xmin,
                FloatDefault xmax,
                FloatDefault ymin,
                FloatDefault ymax,
                FloatDefault zmin,
                FloatDefault zmax)
{
  MinPoint[0] = xmin;
  MaxPoint[0] = xmax;
  MinPoint[1] = ymin;
  MaxPoint[1] = ymax;
  MinPoint[2] = zmin;
  MaxPoint[2] = zmax;
}

inline void Box::SetMinPoint(const vtkm::Vec<FloatDefault, 3>& point)
{
  this->MinPoint = point;
  this->Modified();
}

inline void Box::SetMaxPoint(const vtkm::Vec<FloatDefault, 3>& point)
{
  this->MaxPoint = point;
  this->Modified();
}

inline const vtkm::Vec<FloatDefault, 3>& Box::GetMinPoint() const
{
  return this->MinPoint;
}

inline const vtkm::Vec<FloatDefault, 3>& Box::GetMaxPoint() const
{
  return this->MaxPoint;
}

VTKM_EXEC_CONT
inline FloatDefault Box::Value(FloatDefault x, FloatDefault y, FloatDefault z) const
{
  return this->Value(vtkm::Vec<vtkm::FloatDefault, 3>(x, y, z));
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Box::Gradient(FloatDefault x,
                                                FloatDefault y,
                                                FloatDefault z) const
{
  return this->Gradient(vtkm::Vec<FloatDefault, 3>(x, y, z));
}

VTKM_EXEC_CONT
inline FloatDefault Box::Value(const vtkm::Vec<FloatDefault, 3>& x) const
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
      distance += dist * dist;
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

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Box::Gradient(const vtkm::Vec<FloatDefault, 3>& x) const
{
  vtkm::IdComponent minAxis = 0;
  FloatDefault dist = 0.0;
  FloatDefault minDist = vtkm::Infinity32();
  vtkm::Vec<vtkm::IdComponent, 3> location;
  vtkm::Vec<FloatDefault, 3> normal(FloatDefault(0));
  vtkm::Vec<FloatDefault, 3> inside(FloatDefault(0));
  vtkm::Vec<FloatDefault, 3> outside(FloatDefault(0));
  vtkm::Vec<FloatDefault, 3> center((this->MaxPoint + this->MinPoint) * FloatDefault(0.5));

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

  vtkm::Id indx = location[0] + 3 * location[1] + 9 * location[2];
  switch (indx)
  {
    // verts - gradient points away from center point
    case 0:
    case 2:
    case 6:
    case 8:
    case 18:
    case 20:
    case 24:
    case 26:
      for (vtkm::IdComponent d = 0; d < 3; d++)
      {
        normal[d] = x[d] - center[d];
      }
      vtkm::Normalize(normal);
      break;

    // edges - gradient points out from axis of cube
    case 1:
    case 3:
    case 5:
    case 7:
    case 9:
    case 11:
    case 15:
    case 17:
    case 19:
    case 21:
    case 23:
    case 25:
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
    case 4:
    case 10:
    case 12:
    case 14:
    case 16:
    case 22:
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

//============================================================================
inline Cylinder::Cylinder()
  : Center(FloatDefault(0))
  , Axis(vtkm::make_Vec(FloatDefault(1), FloatDefault(0), FloatDefault(0)))
  , Radius(FloatDefault(0.2))
{
}

inline Cylinder::Cylinder(const vtkm::Vec<FloatDefault, 3>& axis, FloatDefault radius)
  : Center(FloatDefault(0))
  , Axis(vtkm::Normal(axis))
  , Radius(radius)
{
}

inline Cylinder::Cylinder(const vtkm::Vec<FloatDefault, 3>& center,
                          const vtkm::Vec<FloatDefault, 3>& axis,
                          FloatDefault radius)
  : Center(center)
  , Axis(vtkm::Normal(axis))
  , Radius(radius)
{
}

inline void Cylinder::SetCenter(const vtkm::Vec<FloatDefault, 3>& center)
{
  this->Center = center;
  this->Modified();
}

inline void Cylinder::SetAxis(const vtkm::Vec<FloatDefault, 3>& axis)
{
  this->Axis = vtkm::Normal(axis);
  this->Modified();
}

inline void Cylinder::SetRadius(FloatDefault radius)
{
  this->Radius = radius;
  this->Modified();
}

inline const vtkm::Vec<FloatDefault, 3>& Cylinder::GetCenter() const
{
  return this->Center;
}

inline const vtkm::Vec<FloatDefault, 3>& Cylinder::GetAxis() const
{
  return this->Axis;
}

inline FloatDefault Cylinder::GetRadius() const
{
  return this->Radius;
}

VTKM_EXEC_CONT
inline FloatDefault Cylinder::Value(const vtkm::Vec<FloatDefault, 3>& x) const
{
  vtkm::Vec<FloatDefault, 3> x2c = x - this->Center;
  FloatDefault proj = vtkm::dot(this->Axis, x2c);
  return vtkm::dot(x2c, x2c) - (proj * proj) - (this->Radius * this->Radius);
}

VTKM_EXEC_CONT
inline FloatDefault Cylinder::Value(FloatDefault x, FloatDefault y, FloatDefault z) const
{
  return this->Value(vtkm::Vec<vtkm::FloatDefault, 3>(x, y, z));
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Cylinder::Gradient(const vtkm::Vec<FloatDefault, 3>& x) const
{
  vtkm::Vec<FloatDefault, 3> x2c = x - this->Center;
  FloatDefault t = this->Axis[0] * x2c[0] + this->Axis[1] * x2c[1] + this->Axis[2] * x2c[2];
  vtkm::Vec<FloatDefault, 3> closestPoint = this->Center + (this->Axis * t);
  return (x - closestPoint) * FloatDefault(2);
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Cylinder::Gradient(FloatDefault x,
                                                     FloatDefault y,
                                                     FloatDefault z) const
{
  return this->Gradient(vtkm::Vec<FloatDefault, 3>(x, y, z));
}

//============================================================================
inline Frustum::Frustum()
{
  std::fill(this->Points, this->Points + 6, vtkm::Vec<FloatDefault, 3>{});
  std::fill(this->Normals, this->Normals + 6, vtkm::Vec<FloatDefault, 3>{});
}

inline Frustum::Frustum(const vtkm::Vec<FloatDefault, 3> points[6],
                        const vtkm::Vec<FloatDefault, 3> normals[6])
{
  std::copy(points, points + 6, this->Points);
  std::copy(normals, normals + 6, this->Normals);
}

inline Frustum::Frustum(const vtkm::Vec<FloatDefault, 3> points[8])
{
  this->CreateFromPoints(points);
}

inline void Frustum::SetPlanes(const vtkm::Vec<FloatDefault, 3> points[6],
                               const vtkm::Vec<FloatDefault, 3> normals[6])
{
  std::copy(points, points + 6, this->Points);
  std::copy(normals, normals + 6, this->Normals);
  this->Modified();
}

inline void Frustum::GetPlanes(vtkm::Vec<FloatDefault, 3> points[6],
                               vtkm::Vec<FloatDefault, 3> normals[6]) const
{
  std::copy(this->Points, this->Points + 6, points);
  std::copy(this->Normals, this->Normals + 6, normals);
}

inline const vtkm::Vec<FloatDefault, 3>* Frustum::GetPoints() const
{
  return this->Points;
}

inline const vtkm::Vec<FloatDefault, 3>* Frustum::GetNormals() const
{
  return this->Normals;
}

inline void Frustum::SetPlane(int idx,
                              vtkm::Vec<FloatDefault, 3>& point,
                              vtkm::Vec<FloatDefault, 3>& normal)
{
  if (idx < 0 || idx >= 6)
  {
    std::string msg = "Plane idx ";
    msg += std::to_string(idx) + " is out of range [0, 5]";
    throw vtkm::cont::ErrorBadValue(msg);
  }

  this->Points[idx] = point;
  this->Normals[idx] = normal;
  this->Modified();
}

inline void Frustum::CreateFromPoints(const vtkm::Vec<FloatDefault, 3> points[8])
{
  // XXX(clang-format-3.9): 3.8 is silly. 3.9 makes it look like this.
  // clang-format off
  int planes[6][3] = {
    { 3, 2, 0 }, { 4, 5, 7 }, { 0, 1, 4 }, { 1, 2, 5 }, { 2, 3, 6 }, { 3, 0, 7 }
  };
  // clang-format on

  for (int i = 0; i < 6; ++i)
  {
    auto& v0 = points[planes[i][0]];
    auto& v1 = points[planes[i][1]];
    auto& v2 = points[planes[i][2]];

    this->Points[i] = v0;
    this->Normals[i] = vtkm::Normal(vtkm::Cross(v2 - v0, v1 - v0));
    this->Modified();
  }
}

VTKM_EXEC_CONT
inline FloatDefault Frustum::Value(FloatDefault x, FloatDefault y, FloatDefault z) const
{
  FloatDefault maxVal = -std::numeric_limits<FloatDefault>::max();
  for (int i = 0; i < 6; ++i)
  {
    auto& p = this->Points[i];
    auto& n = this->Normals[i];
    FloatDefault val = ((x - p[0]) * n[0]) + ((y - p[1]) * n[1]) + ((z - p[2]) * n[2]);
    maxVal = vtkm::Max(maxVal, val);
  }
  return maxVal;
}

VTKM_EXEC_CONT
inline FloatDefault Frustum::Value(const vtkm::Vec<FloatDefault, 3>& x) const
{
  return this->Value(x[0], x[1], x[2]);
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Frustum::Gradient(FloatDefault x,
                                                    FloatDefault y,
                                                    FloatDefault z) const
{
  FloatDefault maxVal = -std::numeric_limits<FloatDefault>::max();
  int maxValIdx = 0;
  for (int i = 0; i < 6; ++i)
  {
    auto& p = this->Points[i];
    auto& n = this->Normals[i];
    FloatDefault val = ((x - p[0]) * n[0]) + ((y - p[1]) * n[1]) + ((z - p[2]) * n[2]);
    if (val > maxVal)
    {
      maxVal = val;
      maxValIdx = i;
    }
  }
  return this->Normals[maxValIdx];
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Frustum::Gradient(const vtkm::Vec<FloatDefault, 3>& x) const
{
  return this->Gradient(x[0], x[1], x[2]);
}

//============================================================================
inline Plane::Plane()
  : Origin(FloatDefault(0))
  , Normal(FloatDefault(0), FloatDefault(0), FloatDefault(1))
{
}

inline Plane::Plane(const vtkm::Vec<FloatDefault, 3>& normal)
  : Origin(FloatDefault(0))
  , Normal(normal)
{
}

inline Plane::Plane(const vtkm::Vec<FloatDefault, 3>& origin,
                    const vtkm::Vec<FloatDefault, 3>& normal)
  : Origin(origin)
  , Normal(normal)
{
}

inline void Plane::SetOrigin(const vtkm::Vec<FloatDefault, 3>& origin)
{
  this->Origin = origin;
  this->Modified();
}

inline void Plane::SetNormal(const vtkm::Vec<FloatDefault, 3>& normal)
{
  this->Normal = normal;
  this->Modified();
}

inline const vtkm::Vec<FloatDefault, 3>& Plane::GetOrigin() const
{
  return this->Origin;
}

inline const vtkm::Vec<FloatDefault, 3>& Plane::GetNormal() const
{
  return this->Normal;
}

VTKM_EXEC_CONT
inline FloatDefault Plane::Value(FloatDefault x, FloatDefault y, FloatDefault z) const
{
  return ((x - this->Origin[0]) * this->Normal[0]) + ((y - this->Origin[1]) * this->Normal[1]) +
    ((z - this->Origin[2]) * this->Normal[2]);
}

VTKM_EXEC_CONT
inline FloatDefault Plane::Value(const vtkm::Vec<FloatDefault, 3>& x) const
{
  return this->Value(x[0], x[1], x[2]);
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Plane::Gradient(FloatDefault, FloatDefault, FloatDefault) const
{
  return this->Normal;
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Plane::Gradient(const vtkm::Vec<FloatDefault, 3>&) const
{
  return this->Normal;
}

//============================================================================
inline Sphere::Sphere()
  : Radius(FloatDefault(0.2))
  , Center(FloatDefault(0))
{
}

inline Sphere::Sphere(FloatDefault radius)
  : Radius(radius)
  , Center(FloatDefault(0))
{
}

inline Sphere::Sphere(vtkm::Vec<FloatDefault, 3> center, FloatDefault radius)
  : Radius(radius)
  , Center(center)
{
}

inline void Sphere::SetRadius(FloatDefault radius)
{
  this->Radius = radius;
  this->Modified();
}

inline void Sphere::SetCenter(const vtkm::Vec<FloatDefault, 3>& center)
{
  this->Center = center;
  this->Modified();
}

inline FloatDefault Sphere::GetRadius() const
{
  return this->Radius;
}

inline const vtkm::Vec<FloatDefault, 3>& Sphere::GetCenter() const
{
  return this->Center;
}

VTKM_EXEC_CONT
inline FloatDefault Sphere::Value(FloatDefault x, FloatDefault y, FloatDefault z) const
{
  return ((x - this->Center[0]) * (x - this->Center[0]) +
          (y - this->Center[1]) * (y - this->Center[1]) +
          (z - this->Center[2]) * (z - this->Center[2])) -
    (this->Radius * this->Radius);
}

VTKM_EXEC_CONT
inline FloatDefault Sphere::Value(const vtkm::Vec<FloatDefault, 3>& x) const
{
  return this->Value(x[0], x[1], x[2]);
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Sphere::Gradient(FloatDefault x,
                                                   FloatDefault y,
                                                   FloatDefault z) const
{
  return this->Gradient(vtkm::Vec<FloatDefault, 3>(x, y, z));
}

VTKM_EXEC_CONT
inline vtkm::Vec<FloatDefault, 3> Sphere::Gradient(const vtkm::Vec<FloatDefault, 3>& x) const
{
  return FloatDefault(2) * (x - this->Center);
}
}
} // vtkm::cont
