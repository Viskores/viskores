//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_TriagnleIntersector_h
#define vtk_m_rendering_raytracing_TriagnleIntersector_h
#include <cstring>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace
{

enum : vtkm::Int32
{
  END_FLAG2 = -1000000000
};
}

template <typename TriIntersector>
class TriLeafIntersector
{
public:
  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VTKM_EXEC inline void IntersectLeaf(const vtkm::Int32& currentNode,
                                      const Precision& originX,
                                      const Precision& originY,
                                      const Precision& originZ,
                                      const Precision& dirx,
                                      const Precision& diry,
                                      const Precision& dirz,
                                      const PointPortalType& points,
                                      vtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& minU,
                                      Precision& minV,
                                      LeafPortalType Leafs,
                                      const Precision& minDistance) const
  {
    vtkm::Vec<Int32, 4> leafnode = Leafs.Get(currentNode);
    vtkm::Vec<Precision, 3> a = vtkm::Vec<Precision, 3>(points.Get(leafnode[1]));
    vtkm::Vec<Precision, 3> b = vtkm::Vec<Precision, 3>(points.Get(leafnode[2]));
    vtkm::Vec<Precision, 3> c = vtkm::Vec<Precision, 3>(points.Get(leafnode[3]));
    TriIntersector intersector;
    Precision distance = -1.;
    Precision u, v;

    intersector.IntersectTri(a, b, c, dirx, diry, dirz, distance, u, v, originX, originY, originZ);

    if (distance != -1. && distance < closestDistance && distance > minDistance)
    {
      closestDistance = distance;
      minU = u;
      minV = v;
      hitIndex = currentNode;
    }
  }
};

class Moller
{
public:
  template <typename Precision>
  VTKM_EXEC void IntersectTri(const vtkm::Vec<Precision, 3>& a,
                              const vtkm::Vec<Precision, 3>& b,
                              const vtkm::Vec<Precision, 3>& c,
                              const Precision& dirx,
                              const Precision& diry,
                              const Precision& dirz,
                              Precision& distance,
                              Precision& u,
                              Precision& v,
                              const Precision& originX,
                              const Precision& originY,
                              const Precision& originZ) const
  {
    const vtkm::Float32 EPSILON2 = 0.0001f;

    vtkm::Vec<Precision, 3> e1 = b - a;
    vtkm::Vec<Precision, 3> e2 = c - a;

    vtkm::Vec<Precision, 3> p;
    p[0] = diry * e2[2] - dirz * e2[1];
    p[1] = dirz * e2[0] - dirx * e2[2];
    p[2] = dirx * e2[1] - diry * e2[0];
    Precision dot = e1[0] * p[0] + e1[1] * p[1] + e1[2] * p[2];
    if (dot != 0.f)
    {
      dot = 1.f / dot;
      vtkm::Vec<Precision, 3> t;
      t[0] = originX - a[0];
      t[1] = originY - a[1];
      t[2] = originZ - a[2];

      u = (t[0] * p[0] + t[1] * p[1] + t[2] * p[2]) * dot;
      if (u >= (0.f - EPSILON2) && u <= (1.f + EPSILON2))
      {

        vtkm::Vec<Precision, 3> q; // = t % e1;
        q[0] = t[1] * e1[2] - t[2] * e1[1];
        q[1] = t[2] * e1[0] - t[0] * e1[2];
        q[2] = t[0] * e1[1] - t[1] * e1[0];
        v = (dirx * q[0] + diry * q[1] + dirz * q[2]) * dot;
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


template <typename Precision>
class WaterTight
{
public:
  VTKM_EXEC
  inline void FindDir(const vtkm::Vec<Precision, 3>& dir,
                      Precision& sx,
                      Precision& sy,
                      Precision& sz,
                      vtkm::Int32& kx,
                      vtkm::Int32& ky,
                      vtkm::Int32& kz) const
  {
    //Find max ray direction
    kz = 0;
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

    kx = kz + 1;
    if (kx == 3)
      kx = 0;
    ky = kx + 1;
    if (ky == 3)
      ky = 0;

    if (dir[kz] < 0.f)
    {
      vtkm::Int32 temp = ky;
      ky = kx;
      kx = temp;
    }

    sx = dir[kx] / dir[kz];
    sy = dir[ky] / dir[kz];
    sz = 1.f / dir[kz];
  }

  VTKM_EXEC_CONT
  inline void IntersectTri(const vtkm::Vec<Precision, 3>& a,
                           const vtkm::Vec<Precision, 3>& b,
                           const vtkm::Vec<Precision, 3>& c,
                           const Precision& dirx,
                           const Precision& diry,
                           const Precision& dirz,
                           Precision& distance,
                           Precision& u,
                           Precision& v,
                           const Precision& originX,
                           const Precision& originY,
                           const Precision& originZ) const
  {
    vtkm::Vec<Precision, 3> dir;
    dir[0] = dirx;
    dir[1] = diry;
    dir[2] = dirz;
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

    Precision Sx = dir[kx] / dir[kz];
    Precision Sy = dir[ky] / dir[kz];
    Precision Sz = 1.f / dir[kz];



    vtkm::Vec<Precision, 3> A, B, C;
    A[0] = a[0] - originX;
    A[1] = a[1] - originY;
    A[2] = a[2] - originZ;
    B[0] = b[0] - originX;
    B[1] = b[1] - originY;
    B[2] = b[2] - originZ;
    C[0] = c[0] - originX;
    C[1] = c[1] - originY;
    C[2] = c[2] - originZ;

    const Precision Ax = A[kx] - Sx * A[kz];
    const Precision Ay = A[ky] - Sy * A[kz];
    const Precision Bx = B[kx] - Sx * B[kz];
    const Precision By = B[ky] - Sy * B[kz];
    const Precision Cx = C[kx] - Sx * C[kz];
    const Precision Cy = C[ky] - Sy * C[kz];

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

    const Precision Az = Sz * A[kz];
    const Precision Bz = Sz * B[kz];
    const Precision Cz = Sz * C[kz];

    det = 1.f / det;

    u = u * det;
    v = v * det;

    distance = (u * Az + v * Bz + w * det * Cz);
    u = v;
    v = w * det;
    if (invalid)
      distance = -1.;
  }

  VTKM_EXEC
  inline void IntersectTriSn(const vtkm::Vec<Precision, 3>& a,
                             const vtkm::Vec<Precision, 3>& b,
                             const vtkm::Vec<Precision, 3>& c,
                             const Precision& sx,
                             const Precision& sy,
                             const Precision& sz,
                             const vtkm::Int32& kx,
                             const vtkm::Int32& ky,
                             const vtkm::Int32& kz,
                             Precision& distance,
                             Precision& u,
                             Precision& v,
                             const Precision& originX,
                             const Precision& originY,
                             const Precision& originZ) const
  {



    vtkm::Vec<Precision, 3> A, B, C;
    A[0] = a[0] - originX;
    A[1] = a[1] - originY;
    A[2] = a[2] - originZ;
    B[0] = b[0] - originX;
    B[1] = b[1] - originY;
    B[2] = b[2] - originZ;
    C[0] = c[0] - originX;
    C[1] = c[1] - originY;
    C[2] = c[2] - originZ;

    const Precision Ax = A[kx] - sx * A[kz];
    const Precision Ay = A[ky] - sy * A[kz];
    const Precision Bx = B[kx] - sx * B[kz];
    const Precision By = B[ky] - sy * B[kz];
    const Precision Cx = C[kx] - sx * C[kz];
    const Precision Cy = C[ky] - sy * C[kz];

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

    const Precision Az = sz * A[kz];
    const Precision Bz = sz * B[kz];
    const Precision Cz = sz * C[kz];

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

//
// Double precision specialization
//
template <>
class WaterTight<vtkm::Float64>
{
public:
  VTKM_EXEC
  inline void FindDir(const vtkm::Vec<vtkm::Float64, 3>& dir,
                      vtkm::Float64& sx,
                      vtkm::Float64& sy,
                      vtkm::Float64& sz,
                      vtkm::Int32& kx,
                      vtkm::Int32& ky,
                      vtkm::Int32& kz) const
  {
    //Find max ray direction
    kz = 0;
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

    kx = kz + 1;
    if (kx == 3)
      kx = 0;
    ky = kx + 1;
    if (ky == 3)
      ky = 0;

    if (dir[kz] < 0.f)
    {
      vtkm::Int32 temp = ky;
      ky = kx;
      kx = temp;
    }

    sx = dir[kx] / dir[kz];
    sy = dir[ky] / dir[kz];
    sz = 1.f / dir[kz];
  }

  VTKM_EXEC
  inline void IntersectTri(const vtkm::Vec<vtkm::Float64, 3>& a,
                           const vtkm::Vec<vtkm::Float64, 3>& b,
                           const vtkm::Vec<vtkm::Float64, 3>& c,
                           const vtkm::Float64& dirx,
                           const vtkm::Float64& diry,
                           const vtkm::Float64& dirz,
                           vtkm::Float64& distance,
                           vtkm::Float64& u,
                           vtkm::Float64& v,
                           const vtkm::Float64& originX,
                           const vtkm::Float64& originY,
                           const vtkm::Float64& originZ) const
  {
    vtkm::Vec<vtkm::Float64, 3> dir;
    dir[0] = dirx;
    dir[1] = diry;
    dir[2] = dirz;
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



    vtkm::Vec<vtkm::Float64, 3> A, B, C;
    A[0] = a[0] - originX;
    A[1] = a[1] - originY;
    A[2] = a[2] - originZ;
    B[0] = b[0] - originX;
    B[1] = b[1] - originY;
    B[2] = b[2] - originZ;
    C[0] = c[0] - originX;
    C[1] = c[1] - originY;
    C[2] = c[2] - originZ;

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

  VTKM_EXEC
  inline void IntersectTriSn(const vtkm::Vec<vtkm::Float64, 3>& a,
                             const vtkm::Vec<vtkm::Float64, 3>& b,
                             const vtkm::Vec<vtkm::Float64, 3>& c,
                             const vtkm::Float64& sx,
                             const vtkm::Float64& sy,
                             const vtkm::Float64& sz,
                             const vtkm::Int32& kx,
                             const vtkm::Int32& ky,
                             const vtkm::Int32& kz,
                             vtkm::Float64& distance,
                             vtkm::Float64& u,
                             vtkm::Float64& v,
                             const vtkm::Float64& originX,
                             const vtkm::Float64& originY,
                             const vtkm::Float64& originZ) const
  {

    vtkm::Vec<vtkm::Float64, 3> A, B, C;
    A[0] = a[0] - originX;
    A[1] = a[1] - originY;
    A[2] = a[2] - originZ;
    B[0] = b[0] - originX;
    B[1] = b[1] - originY;
    B[2] = b[2] - originZ;
    C[0] = c[0] - originX;
    C[1] = c[1] - originY;
    C[2] = c[2] - originZ;

    const vtkm::Float64 Ax = A[kx] - sx * A[kz];
    const vtkm::Float64 Ay = A[ky] - sy * A[kz];
    const vtkm::Float64 Bx = B[kx] - sx * B[kz];
    const vtkm::Float64 By = B[ky] - sy * B[kz];
    const vtkm::Float64 Cx = C[kx] - sx * C[kz];
    const vtkm::Float64 Cy = C[ky] - sy * C[kz];

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

    const vtkm::Float64 Az = sz * A[kz];
    const vtkm::Float64 Bz = sz * B[kz];
    const vtkm::Float64 Cz = sz * C[kz];

    det = 1. / det;

    u = u * det;
    v = v * det;

    distance = (u * Az + v * Bz + w * det * Cz);
    u = v;
    v = w * det;
    if (invalid)
      distance = -1.;
  }

}; //WaterTight

template <typename BVHPortalType, typename RayPrecision>
VTKM_EXEC inline bool IntersectAABB(const BVHPortalType& bvh,
                                    const vtkm::Int32& currentNode,
                                    const RayPrecision& originDirX,
                                    const RayPrecision& originDirY,
                                    const RayPrecision& originDirZ,
                                    const RayPrecision& invDirx,
                                    const RayPrecision& invDiry,
                                    const RayPrecision& invDirz,
                                    const RayPrecision& closestDistance,
                                    bool& hitLeftChild,
                                    bool& hitRightChild,
                                    const RayPrecision& minDistance) //Find hit after this distance
{
  vtkm::Vec<vtkm::Float32, 4> first4 = bvh.Get(currentNode);
  vtkm::Vec<vtkm::Float32, 4> second4 = bvh.Get(currentNode + 1);
  vtkm::Vec<vtkm::Float32, 4> third4 = bvh.Get(currentNode + 2);

  RayPrecision xmin0 = first4[0] * invDirx - originDirX;
  RayPrecision ymin0 = first4[1] * invDiry - originDirY;
  RayPrecision zmin0 = first4[2] * invDirz - originDirZ;
  RayPrecision xmax0 = first4[3] * invDirx - originDirX;
  RayPrecision ymax0 = second4[0] * invDiry - originDirY;
  RayPrecision zmax0 = second4[1] * invDirz - originDirZ;

  RayPrecision min0 = vtkm::Max(
    vtkm::Max(vtkm::Max(vtkm::Min(ymin0, ymax0), vtkm::Min(xmin0, xmax0)), vtkm::Min(zmin0, zmax0)),
    minDistance);
  RayPrecision max0 = vtkm::Min(
    vtkm::Min(vtkm::Min(vtkm::Max(ymin0, ymax0), vtkm::Max(xmin0, xmax0)), vtkm::Max(zmin0, zmax0)),
    closestDistance);
  hitLeftChild = (max0 >= min0);

  RayPrecision xmin1 = second4[2] * invDirx - originDirX;
  RayPrecision ymin1 = second4[3] * invDiry - originDirY;
  RayPrecision zmin1 = third4[0] * invDirz - originDirZ;
  RayPrecision xmax1 = third4[1] * invDirx - originDirX;
  RayPrecision ymax1 = third4[2] * invDiry - originDirY;
  RayPrecision zmax1 = third4[3] * invDirz - originDirZ;

  RayPrecision min1 = vtkm::Max(
    vtkm::Max(vtkm::Max(vtkm::Min(ymin1, ymax1), vtkm::Min(xmin1, xmax1)), vtkm::Min(zmin1, zmax1)),
    minDistance);
  RayPrecision max1 = vtkm::Min(
    vtkm::Min(vtkm::Min(vtkm::Max(ymin1, ymax1), vtkm::Max(xmin1, xmax1)), vtkm::Max(zmin1, zmax1)),
    closestDistance);
  hitRightChild = (max1 >= min1);
  return (min0 > min1);
}

template <typename T>
VTKM_EXEC inline void swap(T& a, T& b)
{
  T tmp = a;
  a = b;
  b = tmp;
}

VTKM_EXEC
inline vtkm::Float32 up(const vtkm::Float32& a)
{
  return (a > 0.f) ? a * (1.f + vtkm::Float32(2e-23)) : a * (1.f - vtkm::Float32(2e-23));
}

VTKM_EXEC
inline vtkm::Float32 down(const vtkm::Float32& a)
{
  return (a > 0.f) ? a * (1.f - vtkm::Float32(2e-23)) : a * (1.f + vtkm::Float32(2e-23));
}

VTKM_EXEC
inline vtkm::Float32 upFast(const vtkm::Float32& a)
{
  return a * (1.f + vtkm::Float32(2e-23));
}

VTKM_EXEC
inline vtkm::Float32 downFast(const vtkm::Float32& a)
{
  return a * (1.f - vtkm::Float32(2e-23));
}

template <typename Device, typename LeafIntesectorType>
class TriangleIntersector
{
public:
  using Float4ArrayHandle = typename vtkm::cont::ArrayHandle<Vec<vtkm::Float32, 4>>;
  using Int2Handle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 2>>;
  using Int4Handle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>;
  using Float4ArrayPortal = typename Float4ArrayHandle::ExecutionTypes<Device>::PortalConst;
  using Int2ArrayPortal = typename Int2Handle::ExecutionTypes<Device>::PortalConst;
  using Int4ArrayPortal = typename Int4Handle::ExecutionTypes<Device>::PortalConst;

  class Intersector : public vtkm::worklet::WorkletMapField
  {
  private:
    LeafIntesectorType LeafIntersector;
    bool Occlusion;
    Float4ArrayPortal FlatBVH;
    Int4ArrayPortal Leafs;
    VTKM_EXEC
    inline vtkm::Float32 rcp(vtkm::Float32 f) const { return 1.0f / f; }
    VTKM_EXEC
    inline vtkm::Float32 rcp_safe(vtkm::Float32 f) const
    {
      return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
    }
    VTKM_EXEC
    inline vtkm::Float64 rcp(vtkm::Float64 f) const { return 1.0 / f; }
    VTKM_EXEC
    inline vtkm::Float64 rcp_safe(vtkm::Float64 f) const
    {
      return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
    }

  public:
    VTKM_CONT
    Intersector(bool occlusion, LinearBVH& bvh)
      : Occlusion(occlusion)
      , FlatBVH(bvh.FlatBVH.PrepareForInput(Device()))
      , Leafs(bvh.LeafNodes.PrepareForInput(Device()))
    {
    }
    using ControlSignature = void(FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  WholeArrayIn<Vec3RenderingTypes>);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);


    template <typename PointPortalType, typename Precision>
    VTKM_EXEC void operator()(const vtkm::Vec<Precision, 3>& rayDir,
                              const vtkm::Vec<Precision, 3>& rayOrigin,
                              Precision& distance,
                              const Precision& minDistance,
                              const Precision& maxDistance,
                              Precision& minU,
                              Precision& minV,
                              vtkm::Id& hitIndex,
                              const PointPortalType& points) const
    {
      Precision closestDistance = maxDistance;
      distance = maxDistance;
      hitIndex = -1;
      Precision dirx = rayDir[0];
      Precision diry = rayDir[1];
      Precision dirz = rayDir[2];

      Precision invDirx = rcp_safe(dirx);
      Precision invDiry = rcp_safe(diry);
      Precision invDirz = rcp_safe(dirz);
      vtkm::Int32 currentNode;

      vtkm::Int32 todo[64];
      vtkm::Int32 stackptr = 0;
      vtkm::Int32 barrier = END_FLAG2;
      currentNode = 0;

      todo[stackptr] = barrier;

      Precision originX = rayOrigin[0];
      Precision originY = rayOrigin[1];
      Precision originZ = rayOrigin[2];
      Precision originDirX = originX * invDirx;
      Precision originDirY = originY * invDiry;
      Precision originDirZ = originZ * invDirz;
      while (currentNode != END_FLAG2)
      {
        if (currentNode > -1)
        {


          bool hitLeftChild, hitRightChild;
          bool rightCloser = IntersectAABB(FlatBVH,
                                           currentNode,
                                           originDirX,
                                           originDirY,
                                           originDirZ,
                                           invDirx,
                                           invDiry,
                                           invDirz,
                                           closestDistance,
                                           hitLeftChild,
                                           hitRightChild,
                                           minDistance);

          if (!hitLeftChild && !hitRightChild)
          {
            currentNode = todo[stackptr];
            stackptr--;
          }
          else
          {
            vtkm::Vec<vtkm::Float32, 4> children =
              FlatBVH.Get(currentNode + 3); //Children.Get(currentNode);
            vtkm::Int32 leftChild;
            memcpy(&leftChild, &children[0], 4);
            vtkm::Int32 rightChild;
            memcpy(&rightChild, &children[1], 4);
            currentNode = (hitLeftChild) ? leftChild : rightChild;
            if (hitLeftChild && hitRightChild)
            {
              if (rightCloser)
              {
                currentNode = rightChild;
                stackptr++;
                todo[stackptr] = leftChild;
              }
              else
              {
                stackptr++;
                todo[stackptr] = rightChild;
              }
            }
          }
        } // if inner node

        if (currentNode < 0 && currentNode != barrier) //check register usage
        {
          currentNode = -currentNode - 1; //swap the neg address
          LeafIntersector.IntersectLeaf(currentNode,
                                        originX,
                                        originY,
                                        originZ,
                                        dirx,
                                        diry,
                                        dirz,
                                        points,
                                        hitIndex,
                                        closestDistance,
                                        minU,
                                        minV,
                                        Leafs,
                                        minDistance);
          currentNode = todo[stackptr];
          stackptr--;
        } // if leaf node

      } //while
      if (hitIndex != -1)
        distance = closestDistance;
    } // ()
  };
  template <typename Precision>
  class IntersectorHitIndex : public vtkm::worklet::WorkletMapField
  {
  private:
    bool Occlusion;
    Float4ArrayPortal FlatBVH;
    Int4ArrayPortal Leafs;
    LeafIntesectorType LeafIntersector;

    VTKM_EXEC
    inline vtkm::Float32 rcp(vtkm::Float32 f) const { return 1.0f / f; }
    VTKM_EXEC
    inline vtkm::Float64 rcp(vtkm::Float64 f) const { return 1.0 / f; }
    VTKM_EXEC
    inline vtkm::Float64 rcp_safe(vtkm::Float64 f) const
    {
      return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
    }
    VTKM_EXEC
    inline vtkm::Float32 rcp_safe(vtkm::Float32 f) const
    {
      return rcp((fabs(f) < 1e-8f) ? 1e-8f : f);
    }

  public:
    VTKM_CONT
    IntersectorHitIndex(bool occlusion, LinearBVH& bvh)
      : Occlusion(occlusion)
      , FlatBVH(bvh.FlatBVH.PrepareForInput(Device()))
      , Leafs(bvh.LeafNodes.PrepareForInput(Device()))
    {
    }
    using ControlSignature = void(FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<Vec3RenderingTypes>,
                                  FieldOut<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
    template <typename PointPortalType>
    VTKM_EXEC void operator()(const vtkm::Vec<Precision, 3>& rayDir,
                              vtkm::Id& hitIndex,
                              const PointPortalType& points,
                              Precision& distance,
                              const Precision& minDistance,
                              const Precision& maxDistance,
                              const vtkm::Vec<Precision, 3>& origin) const
    {
      Precision closestDistance = maxDistance;
      hitIndex = -1;
      Precision dirx = rayDir[0];
      Precision diry = rayDir[1];
      Precision dirz = rayDir[2];

      Precision invDirx = rcp_safe(dirx);
      Precision invDiry = rcp_safe(diry);
      Precision invDirz = rcp_safe(dirz);
      int currentNode;

      vtkm::Int32 todo[64];
      vtkm::Int32 stackptr = 0;
      vtkm::Int32 barrier = END_FLAG2;
      currentNode = 0;

      todo[stackptr] = barrier;

      Precision originX = origin[0];
      Precision originY = origin[1];
      Precision originZ = origin[2];
      Precision originDirX = originX * invDirx;
      Precision originDirY = originY * invDiry;
      Precision originDirZ = originZ * invDirz;
      while (currentNode != END_FLAG2)
      {
        if (currentNode > -1)
        {
          bool hitLeftChild, hitRightChild;
          bool rightCloser = IntersectAABB(FlatBVH,
                                           currentNode,
                                           originDirX,
                                           originDirY,
                                           originDirZ,
                                           invDirx,
                                           invDiry,
                                           invDirz,
                                           closestDistance,
                                           hitLeftChild,
                                           hitRightChild,
                                           minDistance);

          if (!hitLeftChild && !hitRightChild)
          {
            currentNode = todo[stackptr];
            stackptr--;
          }
          else
          {
            vtkm::Vec<vtkm::Float32, 4> children =
              FlatBVH.Get(currentNode + 3); //Children.Get(currentNode);
            vtkm::Int32 leftChild;
            memcpy(&leftChild, &children[0], 4);
            vtkm::Int32 rightChild;
            memcpy(&rightChild, &children[1], 4);
            currentNode = (hitLeftChild) ? leftChild : rightChild;
            if (hitLeftChild && hitRightChild)
            {
              if (rightCloser)
              {
                currentNode = rightChild;
                stackptr++;
                todo[stackptr] = leftChild;
              }
              else
              {
                stackptr++;
                todo[stackptr] = rightChild;
              }
            }
          }
        } // if inner node

        if (currentNode < 0 && currentNode != barrier) //check register usage
        {
          currentNode = -currentNode - 1; //swap the neg address
          Precision minU, minV;

          LeafIntersector.IntersectLeaf(currentNode,
                                        originX,
                                        originY,
                                        originZ,
                                        dirx,
                                        diry,
                                        dirz,
                                        points,
                                        hitIndex,
                                        closestDistance,
                                        minU,
                                        minV,
                                        Leafs,
                                        minDistance);

          currentNode = todo[stackptr];
          stackptr--;
        } // if leaf node

      } //while
      if (hitIndex != -1)
        distance = closestDistance;
    } // ()

  }; //class Intersector

  class CellIndexFilter : public vtkm::worklet::WorkletMapField
  {
  protected:
    Int4ArrayPortal Leafs;

  public:
    VTKM_CONT
    CellIndexFilter(LinearBVH& bvh)
      : Leafs(bvh.LeafNodes.PrepareForInput(Device()))
    {
    }
    using ControlSignature = void(FieldInOut<>);
    using ExecutionSignature = void(_1);
    VTKM_EXEC
    void operator()(vtkm::Id& hitIndex) const
    {
      vtkm::Id cellIndex = -1;
      if (hitIndex != -1)
      {
        cellIndex = Leafs.Get(hitIndex)[0];
      }

      hitIndex = cellIndex;
    }
  }; //class CellIndexFilter

  template <typename DynamicCoordType, typename Precision>
  VTKM_CONT void run(Ray<Precision>& rays, LinearBVH& bvh, DynamicCoordType coordsHandle)
  {
    vtkm::worklet::DispatcherMapField<Intersector, Device>(Intersector(false, bvh))
      .Invoke(rays.Dir,
              rays.Origin,
              rays.Distance,
              rays.MinDistance,
              rays.MaxDistance,
              rays.U,
              rays.V,
              rays.HitIdx,
              coordsHandle);
  }

  template <typename DynamicCoordType, typename Precision>
  VTKM_CONT void runHitOnly(Ray<Precision>& rays,
                            LinearBVH& bvh,
                            DynamicCoordType coordsHandle,
                            bool returnCellIndex)
  {

    vtkm::worklet::DispatcherMapField<IntersectorHitIndex<Precision>, Device>(
      IntersectorHitIndex<Precision>(false, bvh))
      .Invoke(rays.Dir,
              rays.HitIdx,
              coordsHandle,
              rays.Distance,
              rays.MinDistance,
              rays.MaxDistance,
              rays.Origin);
    // Normally we return the index of the triangle hit,
    // but in some cases we are only interested in the cell
    if (returnCellIndex)
    {
      vtkm::worklet::DispatcherMapField<CellIndexFilter, Device>(CellIndexFilter(bvh))
        .Invoke(rays.HitIdx);
    }
    // Update ray status
    RayOperations::UpdateRayStatus(rays, Device());
  }
}; // class intersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_TriagnleIntersector_h
