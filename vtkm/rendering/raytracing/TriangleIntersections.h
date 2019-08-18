//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_TriangleIntersections_h
#define vtk_m_rendering_raytracing_TriangleIntersections_h

#include <vtkm/Math.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class Moller
{
public:
  template <typename Precision>
  VTKM_EXEC void IntersectTri(const vtkm::Vec<Precision, 3>& a,
                              const vtkm::Vec<Precision, 3>& b,
                              const vtkm::Vec<Precision, 3>& c,
                              const vtkm::Vec<Precision, 3>& dir,
                              Precision& distance,
                              Precision& u,
                              Precision& v,
                              const vtkm::Vec<Precision, 3>& origin) const
  {
    const vtkm::Float32 EPSILON2 = 0.0001f;

    vtkm::Vec<Precision, 3> e1 = b - a;
    vtkm::Vec<Precision, 3> e2 = c - a;

    vtkm::Vec<Precision, 3> p;
    p[0] = dir[1] * e2[2] - dir[2] * e2[1];
    p[1] = dir[2] * e2[0] - dir[0] * e2[2];
    p[2] = dir[0] * e2[1] - dir[1] * e2[0];
    Precision dot = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
    if (dot != 0.f)
    {
      dot = 1.f / dot;
      vtkm::Vec<Precision, 3> t;
      t = origin - a;

      u = (t[0] * p[0] + t[1] * p[1] + t[2] * p[2]) * dot;
      if (u >= (0.f - EPSILON2) && u <= (1.f + EPSILON2))
      {

        vtkm::Vec<Precision, 3> q; // = t % e1;
        q[0] = t[1] * e1[2] - t[2] * e1[1];
        q[1] = t[2] * e1[0] - t[0] * e1[2];
        q[2] = t[0] * e1[1] - t[1] * e1[0];
        v = (dir[0] * q[0] + dir[1] * q[1] + dir[2] * q[2]) * dot;
        if (v >= (0.f - EPSILON2) && v <= (1.f + EPSILON2) && !(u + v > 1.f))
        {
          distance = (e2[0] * q[0] + e2[1] * q[1] + e2[2] * q[2]) * dot;
        }
      }
    }
  }

}; //Moller



// TODO: optimization for sorting ray dims before this call.
//       This is called multiple times and kz,kx, and ky are
//       constant for the ray


class WaterTight
{
public:
  template <typename Precision>
  VTKM_EXEC inline void FindDir(const vtkm::Vec<Precision, 3>& dir,
                                vtkm::Vec<Precision, 3>& s,
                                vtkm::Vec<Int32, 3>& k) const
  {
    //Find max ray direction
    k[2] = 0;
    if (vtkm::Abs(dir[0]) > vtkm::Abs(dir[1]))
    {
      if (vtkm::Abs(dir[0]) > vtkm::Abs(dir[2]))
        k[2] = 0;
      else
        k[2] = 2;
    }
    else
    {
      if (vtkm::Abs(dir[1]) > vtkm::Abs(dir[2]))
        k[2] = 1;
      else
        k[2] = 2;
    }

    k[0] = k[2] + 1;
    if (k[0] == 3)
      k[0] = 0;
    k[1] = k[0] + 1;
    if (k[1] == 3)
      k[1] = 0;

    if (dir[k[2]] < 0.f)
    {
      vtkm::Int32 temp = k[1];
      k[1] = k[0];
      k[0] = temp;
    }

    s[0] = dir[k[0]] / dir[k[2]];
    s[1] = dir[k[1]] / dir[k[2]];
    s[2] = 1.f / dir[k[2]];
  }

  template <typename Precision>
  VTKM_EXEC_CONT inline void IntersectTri(const vtkm::Vec<Precision, 3>& a,
                                          const vtkm::Vec<Precision, 3>& b,
                                          const vtkm::Vec<Precision, 3>& c,
                                          const vtkm::Vec<Precision, 3>& dir,
                                          Precision& distance,
                                          Precision& u,
                                          Precision& v,
                                          const vtkm::Vec<Precision, 3>& origin) const
  {
    vtkm::Vec<Int32, 3> k;
    vtkm::Vec<Precision, 3> s;
    //Find max ray direction
    k[2] = 0;
    if (vtkm::Abs(dir[0]) > vtkm::Abs(dir[1]))
    {
      if (vtkm::Abs(dir[0]) > vtkm::Abs(dir[2]))
        k[2] = 0;
      else
        k[2] = 2;
    }
    else
    {
      if (vtkm::Abs(dir[1]) > vtkm::Abs(dir[2]))
        k[2] = 1;
      else
        k[2] = 2;
    }

    k[0] = k[2] + 1;
    if (k[0] == 3)
      k[0] = 0;
    k[1] = k[0] + 1;
    if (k[1] == 3)
      k[1] = 0;

    if (dir[k[2]] < 0.f)
    {
      vtkm::Int32 temp = k[1];
      k[1] = k[0];
      k[0] = temp;
    }

    s[0] = dir[k[0]] / dir[k[2]];
    s[1] = dir[k[1]] / dir[k[2]];
    s[2] = 1.f / dir[k[2]];

    vtkm::Vec<Precision, 3> A, B, C;
    A = a - origin;
    B = b - origin;
    C = c - origin;

    const Precision Ax = A[k[0]] - s[0] * A[k[2]];
    const Precision Ay = A[k[1]] - s[1] * A[k[2]];
    const Precision Bx = B[k[0]] - s[0] * B[k[2]];
    const Precision By = B[k[1]] - s[1] * B[k[2]];
    const Precision Cx = C[k[0]] - s[0] * C[k[2]];
    const Precision Cy = C[k[1]] - s[1] * C[k[2]];

    //scaled barycentric coords
    u = Cx * By - Cy * Bx;
    v = Ax * Cy - Ay * Cx;
    Precision w = Bx * Ay - By * Ax;
    if (u == 0.f || v == 0.f || w == 0.f)
    {
      vtkm::Float64 CxBy = vtkm::Float64(Cx) * vtkm::Float64(By);
      vtkm::Float64 CyBx = vtkm::Float64(Cy) * vtkm::Float64(Bx);
      u = vtkm::Float32(CxBy - CyBx);

      vtkm::Float64 AxCy = vtkm::Float64(Ax) * vtkm::Float64(Cy);
      vtkm::Float64 AyCx = vtkm::Float64(Ay) * vtkm::Float64(Cx);
      v = vtkm::Float32(AxCy - AyCx);

      vtkm::Float64 BxAy = vtkm::Float64(Bx) * vtkm::Float64(Ay);
      vtkm::Float64 ByAx = vtkm::Float64(By) * vtkm::Float64(Ax);
      w = vtkm::Float32(BxAy - ByAx);
    }
    Precision low = vtkm::Min(u, vtkm::Min(v, w));
    Precision high = vtkm::Max(u, vtkm::Max(v, w));

    bool invalid = (low < 0.) && (high > 0.);

    Precision det = u + v + w;

    if (det == 0.)
      invalid = true;

    const Precision Az = s[2] * A[k[2]];
    const Precision Bz = s[2] * B[k[2]];
    const Precision Cz = s[2] * C[k[2]];

    det = 1.f / det;

    u = u * det;
    v = v * det;

    distance = (u * Az + v * Bz + w * det * Cz);
    u = v;
    v = w * det;
    if (invalid)
      distance = -1.;
  }

  template <typename Precision>
  VTKM_EXEC inline void IntersectTriSn(const vtkm::Vec<Precision, 3>& a,
                                       const vtkm::Vec<Precision, 3>& b,
                                       const vtkm::Vec<Precision, 3>& c,
                                       const vtkm::Vec<Precision, 3>& s,
                                       const vtkm::Vec<Int32, 3>& k,
                                       Precision& distance,
                                       Precision& u,
                                       Precision& v,
                                       const vtkm::Vec<Precision, 3>& origin) const
  {
    vtkm::Vec<Precision, 3> A, B, C;
    A = a - origin;
    B = b - origin;
    C = c - origin;

    const Precision Ax = A[k[0]] - s[0] * A[k[2]];
    const Precision Ay = A[k[1]] - s[1] * A[k[2]];
    const Precision Bx = B[k[0]] - s[0] * B[k[2]];
    const Precision By = B[k[1]] - s[1] * B[k[2]];
    const Precision Cx = C[k[0]] - s[0] * C[k[2]];
    const Precision Cy = C[k[1]] - s[1] * C[k[2]];

    //scaled barycentric coords
    u = Cx * By - Cy * Bx;
    v = Ax * Cy - Ay * Cx;
    Precision w = Bx * Ay - By * Ax;
    if (u == 0.f || v == 0.f || w == 0.f)
    {
      vtkm::Float64 CxBy = vtkm::Float64(Cx) * vtkm::Float64(By);
      vtkm::Float64 CyBx = vtkm::Float64(Cy) * vtkm::Float64(Bx);
      u = vtkm::Float32(CxBy - CyBx);

      vtkm::Float64 AxCy = vtkm::Float64(Ax) * vtkm::Float64(Cy);
      vtkm::Float64 AyCx = vtkm::Float64(Ay) * vtkm::Float64(Cx);
      v = vtkm::Float32(AxCy - AyCx);

      vtkm::Float64 BxAy = vtkm::Float64(Bx) * vtkm::Float64(Ay);
      vtkm::Float64 ByAx = vtkm::Float64(By) * vtkm::Float64(Ax);
      w = vtkm::Float32(BxAy - ByAx);
    }

    Precision low = vtkm::Min(u, vtkm::Min(v, w));
    Precision high = vtkm::Max(u, vtkm::Max(v, w));

    bool invalid = (low < 0.) && (high > 0.);

    Precision det = u + v + w;

    if (det == 0.)
      invalid = true;

    const Precision Az = s[2] * A[k[2]];
    const Precision Bz = s[2] * B[k[2]];
    const Precision Cz = s[2] * C[k[2]];

    det = 1.f / det;

    u = u * det;
    v = v * det;

    distance = (u * Az + v * Bz + w * det * Cz);
    u = v;
    v = w * det;
    if (invalid)
      distance = -1.;
  }
}; //WaterTight

template <>
VTKM_EXEC inline void WaterTight::IntersectTri<vtkm::Float64>(const vtkm::Vec3f_64& a,
                                                              const vtkm::Vec3f_64& b,
                                                              const vtkm::Vec3f_64& c,
                                                              const vtkm::Vec3f_64& dir,
                                                              vtkm::Float64& distance,
                                                              vtkm::Float64& u,
                                                              vtkm::Float64& v,
                                                              const vtkm::Vec3f_64& origin) const
{
  //Find max ray direction
  int kz = 0;
  if (vtkm::Abs(dir[0]) > vtkm::Abs(dir[1]))
  {
    if (vtkm::Abs(dir[0]) > vtkm::Abs(dir[2]))
      kz = 0;
    else
      kz = 2;
  }
  else
  {
    if (vtkm::Abs(dir[1]) > vtkm::Abs(dir[2]))
      kz = 1;
    else
      kz = 2;
  }

  vtkm::Int32 kx = kz + 1;
  if (kx == 3)
    kx = 0;
  vtkm::Int32 ky = kx + 1;
  if (ky == 3)
    ky = 0;

  if (dir[kz] < 0.f)
  {
    vtkm::Int32 temp = ky;
    ky = kx;
    kx = temp;
  }

  vtkm::Float64 Sx = dir[kx] / dir[kz];
  vtkm::Float64 Sy = dir[ky] / dir[kz];
  vtkm::Float64 Sz = 1. / dir[kz];



  vtkm::Vec3f_64 A, B, C;
  A = a - origin;
  B = b - origin;
  C = c - origin;

  const vtkm::Float64 Ax = A[kx] - Sx * A[kz];
  const vtkm::Float64 Ay = A[ky] - Sy * A[kz];
  const vtkm::Float64 Bx = B[kx] - Sx * B[kz];
  const vtkm::Float64 By = B[ky] - Sy * B[kz];
  const vtkm::Float64 Cx = C[kx] - Sx * C[kz];
  const vtkm::Float64 Cy = C[ky] - Sy * C[kz];

  //scaled barycentric coords
  u = Cx * By - Cy * Bx;
  v = Ax * Cy - Ay * Cx;

  vtkm::Float64 w = Bx * Ay - By * Ax;

  vtkm::Float64 low = vtkm::Min(u, vtkm::Min(v, w));
  vtkm::Float64 high = vtkm::Max(u, vtkm::Max(v, w));
  bool invalid = (low < 0.) && (high > 0.);

  vtkm::Float64 det = u + v + w;

  if (det == 0.)
    invalid = true;

  const vtkm::Float64 Az = Sz * A[kz];
  const vtkm::Float64 Bz = Sz * B[kz];
  const vtkm::Float64 Cz = Sz * C[kz];

  det = 1. / det;

  u = u * det;
  v = v * det;

  distance = (u * Az + v * Bz + w * det * Cz);
  u = v;
  v = w * det;
  if (invalid)
    distance = -1.;
}
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_TriagnleIntersections_h
