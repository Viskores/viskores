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
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/BVHTraverser.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/ShapeIntersector.h>
#include <vtkm/worklet/WorkletMapField.h>

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
VTKM_EXEC inline void WaterTight::IntersectTri<vtkm::Float64>(
  const vtkm::Vec<vtkm::Float64, 3>& a,
  const vtkm::Vec<vtkm::Float64, 3>& b,
  const vtkm::Vec<vtkm::Float64, 3>& c,
  const vtkm::Vec<vtkm::Float64, 3>& dir,
  vtkm::Float64& distance,
  vtkm::Float64& u,
  vtkm::Float64& v,
  const vtkm::Vec<vtkm::Float64, 3>& origin) const
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



  vtkm::Vec<vtkm::Float64, 3> A, B, C;
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

template <typename Device>
//class WaterTightTriLeafIntersector : public vtkm::exec::ExecutionObjectBase
class WaterTightTriLeafIntersector
{
public:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using Id4ArrayPortal = typename Id4Handle::ExecutionTypes<Device>::PortalConst;
  Id4ArrayPortal Triangles;

public:
  WaterTightTriLeafIntersector(const Id4Handle& triangles)
    : Triangles(triangles.PrepareForInput(Device()))
  {
  }

  WaterTightTriLeafIntersector(const WaterTightTriLeafIntersector<Device>& other)
    : Triangles(other.Triangles)
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VTKM_EXEC inline void IntersectLeaf(const vtkm::Int32& currentNode,
                                      const vtkm::Vec<Precision, 3>& origin,
                                      const vtkm::Vec<Precision, 3>& dir,
                                      const PointPortalType& points,
                                      vtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& minU,
                                      Precision& minV,
                                      LeafPortalType leafs,
                                      const Precision& minDistance) const
  {
    const vtkm::Id triangleCount = leafs.Get(currentNode);
    WaterTight intersector;
    for (vtkm::Id i = 1; i <= triangleCount; ++i)
    {
      const vtkm::Id triIndex = leafs.Get(currentNode + i);
      vtkm::Vec<Id, 4> triangle = Triangles.Get(triIndex);
      vtkm::Vec<Precision, 3> a = vtkm::Vec<Precision, 3>(points.Get(triangle[1]));
      vtkm::Vec<Precision, 3> b = vtkm::Vec<Precision, 3>(points.Get(triangle[2]));
      vtkm::Vec<Precision, 3> c = vtkm::Vec<Precision, 3>(points.Get(triangle[3]));
      Precision distance = -1.;
      Precision u, v;

      intersector.IntersectTri(a, b, c, dir, distance, u, v, origin);
      if (distance != -1. && distance < closestDistance && distance > minDistance)
      {
        closestDistance = distance;
        minU = u;
        minV = v;
        hitIndex = triIndex;
      }
    } // for
  }
};

template <typename Device>
//class MollerTriLeafIntersector : public vtkm::exec::ExecutionObjectBase
class MollerTriLeafIntersector
{
  //protected:
public:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>;
  using Id4ArrayPortal = typename Id4Handle::ExecutionTypes<Device>::PortalConst;
  Id4ArrayPortal Triangles;

public:
  MollerTriLeafIntersector(const Id4Handle& triangles)
    : Triangles(triangles.PrepareForInput(Device()))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VTKM_EXEC inline void IntersectLeaf(const vtkm::Int32& currentNode,
                                      const vtkm::Vec<Precision, 3>& origin,
                                      const vtkm::Vec<Precision, 3>& dir,
                                      const PointPortalType& points,
                                      vtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& minU,
                                      Precision& minV,
                                      LeafPortalType leafs,
                                      const Precision& minDistance) const
  {
    const vtkm::Id triangleCount = leafs.Get(currentNode);
    Moller intersector;
    for (vtkm::Id i = 1; i <= triangleCount; ++i)
    {
      const vtkm::Id triIndex = leafs.Get(currentNode + i);
      vtkm::Vec<Id, 4> triangle = Triangles.Get(triIndex);
      vtkm::Vec<Precision, 3> a = vtkm::Vec<Precision, 3>(points.Get(triangle[1]));
      vtkm::Vec<Precision, 3> b = vtkm::Vec<Precision, 3>(points.Get(triangle[2]));
      vtkm::Vec<Precision, 3> c = vtkm::Vec<Precision, 3>(points.Get(triangle[3]));
      Precision distance = -1.;
      Precision u, v;

      intersector.IntersectTri(a, b, c, dir, distance, u, v, origin);

      if (distance != -1. && distance < closestDistance && distance > minDistance)
      {
        closestDistance = distance;
        minU = u;
        minV = v;
        hitIndex = triIndex;
      }
    } // for
  }
};

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

namespace detail
{

class TriangleIntersectionData
{
public:
  // Worklet to calutate the normals of a triagle if
  // none are stored in the data set
  class CalculateNormals : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    CalculateNormals() {}
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  WholeArrayIn<Vec3RenderingTypes>,
                                  WholeArrayIn<>);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
    template <typename Precision, typename PointPortalType, typename IndicesPortalType>
    VTKM_EXEC inline void operator()(const vtkm::Id& hitIndex,
                                     const vtkm::Vec<Precision, 3>& rayDir,
                                     Precision& normalX,
                                     Precision& normalY,
                                     Precision& normalZ,
                                     const PointPortalType& points,
                                     const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      vtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);
      vtkm::Vec<Precision, 3> a = points.Get(indices[1]);
      vtkm::Vec<Precision, 3> b = points.Get(indices[2]);
      vtkm::Vec<Precision, 3> c = points.Get(indices[3]);

      vtkm::Vec<Precision, 3> normal = vtkm::TriangleNormal(a, b, c);
      vtkm::Normalize(normal);

      //flip the normal if its pointing the wrong way
      if (vtkm::dot(normal, rayDir) > 0.f)
        normal = -normal;
      normalX = normal[0];
      normalY = normal[1];
      normalZ = normal[2];
    }
  }; //class CalculateNormals

  template <typename Precision>
  class LerpScalar : public vtkm::worklet::WorkletMapField
  {
  private:
    Precision MinScalar;
    Precision invDeltaScalar;

  public:
    VTKM_CONT
    LerpScalar(const vtkm::Float32& minScalar, const vtkm::Float32& maxScalar)
      : MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      if (maxScalar - MinScalar != 0.f)
        invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else
        invDeltaScalar = 0.f;
    }
    typedef void ControlSignature(FieldIn<>,
                                  FieldIn<>,
                                  FieldIn<>,
                                  FieldInOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>,
                                  WholeArrayIn<>);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6);
    template <typename ScalarPortalType, typename IndicesPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                              const Precision& u,
                              const Precision& v,
                              Precision& lerpedScalar,
                              const ScalarPortalType& scalars,
                              const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      vtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      Precision n = 1.f - u - v;
      Precision aScalar = Precision(scalars.Get(indices[1]));
      Precision bScalar = Precision(scalars.Get(indices[2]));
      Precision cScalar = Precision(scalars.Get(indices[3]));
      lerpedScalar = aScalar * n + bScalar * u + cScalar * v;
      //normalize
      lerpedScalar = (lerpedScalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar

  template <typename Precision>
  class NodalScalar : public vtkm::worklet::WorkletMapField
  {
  private:
    Precision MinScalar;
    Precision invDeltaScalar;

  public:
    VTKM_CONT
    NodalScalar(const vtkm::Float32& minScalar, const vtkm::Float32& maxScalar)
      : MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      if (maxScalar - MinScalar != 0.f)
        invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else
        invDeltaScalar = 1.f / minScalar;
    }

    typedef void ControlSignature(FieldIn<>,
                                  FieldOut<>,
                                  WholeArrayIn<ScalarRenderingTypes>,
                                  WholeArrayIn<>);

    typedef void ExecutionSignature(_1, _2, _3, _4);
    template <typename ScalarPortalType, typename IndicesPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                              Precision& scalar,
                              const ScalarPortalType& scalars,
                              const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      vtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      //Todo: one normalization
      scalar = Precision(scalars.Get(indices[0]));

      //normalize
      scalar = (scalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar

  template <typename Precision>
  VTKM_CONT void Run(Ray<Precision>& rays,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles,
                     vtkm::cont::CoordinateSystem coordsHandle,
                     const vtkm::cont::Field* scalarField,
                     const vtkm::Range& scalarRange)
  {
    bool isSupportedField =
      (scalarField->GetAssociation() == vtkm::cont::Field::Association::POINTS ||
       scalarField->GetAssociation() == vtkm::cont::Field::Association::CELL_SET);
    if (!isSupportedField)
      throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
    bool isAssocPoints = scalarField->GetAssociation() == vtkm::cont::Field::Association::POINTS;

    // Find the triangle normal
    vtkm::worklet::DispatcherMapField<CalculateNormals>(CalculateNormals())
      .Invoke(
        rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, coordsHandle, triangles);

    // Calculate scalar value at intersection point
    if (isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField<LerpScalar<Precision>>(
        LerpScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx, rays.U, rays.V, rays.Scalar, *scalarField, triangles);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<NodalScalar<Precision>>(
        NodalScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx, rays.Scalar, *scalarField, triangles);
    }
  } // Run

}; // Class IntersectionData

#define AABB_EPSILON 0.00001f
class FindTriangleAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindTriangleAABBs() {}
  typedef void ControlSignature(FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                WholeArrayIn<Vec3RenderingTypes>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Id, 4> indices,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    vtkm::Vec<vtkm::Float32, 3> point;
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(indices[1]));
    xmin = point[0];
    ymin = point[1];
    zmin = point[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(indices[2]));
    xmin = vtkm::Min(xmin, point[0]);
    ymin = vtkm::Min(ymin, point[1]);
    zmin = vtkm::Min(zmin, point[2]);
    xmax = vtkm::Max(xmax, point[0]);
    ymax = vtkm::Max(ymax, point[1]);
    zmax = vtkm::Max(zmax, point[2]);
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(indices[3]));
    xmin = vtkm::Min(xmin, point[0]);
    ymin = vtkm::Min(ymin, point[1]);
    zmin = vtkm::Min(zmin, point[2]);
    xmax = vtkm::Max(xmax, point[0]);
    ymax = vtkm::Max(ymax, point[1]);
    zmax = vtkm::Max(zmax, point[2]);


    vtkm::Float32 xEpsilon, yEpsilon, zEpsilon;
    const vtkm::Float32 minEpsilon = 1e-6f;
    xEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (xmax - xmin));
    yEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (ymax - ymin));
    zEpsilon = vtkm::Max(minEpsilon, AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }
}; //class FindAABBs
#undef AABB_EPSILON

} // namespace detail

class TriangleIntersector : public ShapeIntersector
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> Triangles;
  bool UseWaterTight;

public:
  TriangleIntersector()
    : UseWaterTight(false)
  {
  }

  void SetUseWaterTight(bool useIt) { UseWaterTight = useIt; }

  void SetData(const vtkm::cont::CoordinateSystem& coords,
               vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles)
  {

    CoordsHandle = coords;
    Triangles = triangles;

    vtkm::rendering::raytracing::AABBs AABB;
    vtkm::worklet::DispatcherMapField<detail::FindTriangleAABBs>(detail::FindTriangleAABBs())
      .Invoke(Triangles,
              AABB.xmins,
              AABB.ymins,
              AABB.zmins,
              AABB.xmaxs,
              AABB.ymaxs,
              AABB.zmaxs,
              CoordsHandle);

    this->SetAABBs(AABB);
  }

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> GetTriangles() { return Triangles; }

  class CellIndexFilter : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    CellIndexFilter() {}
    typedef void ControlSignature(FieldInOut<>, WholeArrayIn<>);
    typedef void ExecutionSignature(_1, _2);
    template <typename TrianglePortalType>
    VTKM_EXEC void operator()(vtkm::Id& hitIndex, TrianglePortalType& triangles) const
    {
      vtkm::Id cellIndex = -1;
      if (hitIndex != -1)
      {
        cellIndex = triangles.Get(hitIndex)[0];
      }

      hitIndex = cellIndex;
    }
  }; //class CellIndexFilter

  struct IntersectFunctor
  {
    template <typename Device, typename Precision>
    VTKM_CONT bool operator()(Device,
                              TriangleIntersector* self,
                              Ray<Precision>& rays,
                              bool returnCellIndex)
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(Device);
      self->IntersectRays(rays, Device(), returnCellIndex);
      return true;
    }
  };


  VTKM_CONT void IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex = false) override
  {
    vtkm::cont::TryExecute(IntersectFunctor(), this, rays, returnCellIndex);
  }


  VTKM_CONT void IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex = false) override
  {
    vtkm::cont::TryExecute(IntersectFunctor(), this, rays, returnCellIndex);
  }

  template <typename Precision, typename Device>
  VTKM_CONT void IntersectRays(Ray<Precision>& rays,
                               Device vtkmNotUsed(Device),
                               bool returnCellIndex)
  {

    if (UseWaterTight)
    {
      WaterTightTriLeafIntersector<Device> leafIntersector(this->Triangles);

      BVHTraverser<WaterTightTriLeafIntersector> traverser;
      traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle, Device());
    }
    else
    {
      MollerTriLeafIntersector<Device> leafIntersector(this->Triangles);

      BVHTraverser<MollerTriLeafIntersector> traverser;
      traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle, Device());
    }
    // Normally we return the index of the triangle hit,
    // but in some cases we are only interested in the cell
    if (returnCellIndex)
    {
      vtkm::worklet::DispatcherMapField<CellIndexFilter> cellIndexFilterDispatcher;
      cellIndexFilterDispatcher.Invoke(rays.HitIdx, Triangles);
    }
    // Update ray status
    RayOperations::UpdateRayStatus(rays);
  }

  VTKM_CONT void IntersectionData(Ray<vtkm::Float32>& rays,
                                  const vtkm::cont::Field* scalarField,
                                  const vtkm::Range& scalarRange) override
  {
    IntersectionDataImp(rays, scalarField, scalarRange);
  }

  VTKM_CONT void IntersectionData(Ray<vtkm::Float64>& rays,
                                  const vtkm::cont::Field* scalarField,
                                  const vtkm::Range& scalarRange) override
  {
    IntersectionDataImp(rays, scalarField, scalarRange);
  }

  template <typename Precision>
  VTKM_CONT void IntersectionDataImp(Ray<Precision>& rays,
                                     const vtkm::cont::Field* scalarField,
                                     const vtkm::Range& scalarRange)
  {
    ShapeIntersector::IntersectionPoint(rays);
    detail::TriangleIntersectionData intData;
    intData.Run(rays, this->Triangles, this->CoordsHandle, scalarField, scalarRange);
  }

  vtkm::Id GetNumberOfShapes() const override { return Triangles.GetNumberOfValues(); }
}; // class intersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_TriagnleIntersector_h
