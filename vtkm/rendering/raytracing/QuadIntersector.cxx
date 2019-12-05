//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/rendering/raytracing/BVHTraverser.h>
#include <vtkm/rendering/raytracing/QuadIntersector.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

#define QUAD_AABB_EPSILON 1.0e-4f
class FindQuadAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindQuadAABBs() {}
  typedef void ControlSignature(FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Vec<vtkm::Id, 5> quadId,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    vtkm::Vec3f_32 q, r, s, t;

    q = static_cast<vtkm::Vec3f_32>(points.Get(quadId[1]));
    r = static_cast<vtkm::Vec3f_32>(points.Get(quadId[2]));
    s = static_cast<vtkm::Vec3f_32>(points.Get(quadId[3]));
    t = static_cast<vtkm::Vec3f_32>(points.Get(quadId[4]));

    xmin = q[0];
    ymin = q[1];
    zmin = q[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    xmin = vtkm::Min(xmin, r[0]);
    ymin = vtkm::Min(ymin, r[1]);
    zmin = vtkm::Min(zmin, r[2]);
    xmax = vtkm::Max(xmax, r[0]);
    ymax = vtkm::Max(ymax, r[1]);
    zmax = vtkm::Max(zmax, r[2]);
    xmin = vtkm::Min(xmin, s[0]);
    ymin = vtkm::Min(ymin, s[1]);
    zmin = vtkm::Min(zmin, s[2]);
    xmax = vtkm::Max(xmax, s[0]);
    ymax = vtkm::Max(ymax, s[1]);
    zmax = vtkm::Max(zmax, s[2]);
    xmin = vtkm::Min(xmin, t[0]);
    ymin = vtkm::Min(ymin, t[1]);
    zmin = vtkm::Min(zmin, t[2]);
    xmax = vtkm::Max(xmax, t[0]);
    ymax = vtkm::Max(ymax, t[1]);
    zmax = vtkm::Max(zmax, t[2]);

    vtkm::Float32 xEpsilon, yEpsilon, zEpsilon;
    const vtkm::Float32 minEpsilon = 1e-6f;
    xEpsilon = vtkm::Max(minEpsilon, QUAD_AABB_EPSILON * (xmax - xmin));
    yEpsilon = vtkm::Max(minEpsilon, QUAD_AABB_EPSILON * (ymax - ymin));
    zEpsilon = vtkm::Max(minEpsilon, QUAD_AABB_EPSILON * (zmax - zmin));

    xmin -= xEpsilon;
    ymin -= yEpsilon;
    zmin -= zEpsilon;
    xmax += xEpsilon;
    ymax += yEpsilon;
    zmax += zEpsilon;
  }

}; //class FindAABBs

template <typename Device>
class QuadLeafIntersector
{
public:
  using IdType = vtkm::Vec<vtkm::Id, 5>;
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>>;
  using IdArrayPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  IdArrayPortal QuadIds;

  QuadLeafIntersector() {}

  QuadLeafIntersector(const IdHandle& quadIds)
    : QuadIds(quadIds.PrepareForInput(Device()))
  {
  }

  template <typename vec3, typename Precision>
  VTKM_EXEC bool quad(const vec3& ray_origin,
                      const vec3& ray_direction,
                      const vec3& v00,
                      const vec3& v10,
                      const vec3& v11,
                      const vec3& v01,
                      Precision& u,
                      Precision& v,
                      Precision& t) const
  {

    /* An Eﬃcient Ray-Quadrilateral Intersection Test
         Ares Lagae Philip Dutr´e
         http://graphics.cs.kuleuven.be/publications/LD05ERQIT/index.html

      v01 *------------ * v11
          |\           |
          |  \         |
          |    \       |
          |      \     |
          |        \   |
          |          \ |
      v00 *------------* v10
      */
    // Rejects rays that are parallel to Q, and rays that intersect the plane of
    // Q either on the left of the line V00V01 or on the right of the line V00V10.

    vec3 E03 = v01 - v00;
    vec3 P = vtkm::Cross(ray_direction, E03);
    vec3 E01 = v10 - v00;
    Precision det = vtkm::dot(E01, P);

    if (vtkm::Abs(det) < vtkm::Epsilon<Precision>())
      return false;
    Precision inv_det = 1.0f / det;
    vec3 T = ray_origin - v00;
    Precision alpha = vtkm::dot(T, P) * inv_det;
    if (alpha < 0.0)
      return false;
    vec3 Q = vtkm::Cross(T, E01);
    Precision beta = vtkm::dot(ray_direction, Q) * inv_det;
    if (beta < 0.0)
      return false;

    if ((alpha + beta) > 1.0f)
    {

      // Rejects rays that intersect the plane of Q either on the
      // left of the line V11V10 or on the right of the line V11V01.

      vec3 E23 = v01 - v11;
      vec3 E21 = v10 - v11;
      vec3 P_prime = vtkm::Cross(ray_direction, E21);
      Precision det_prime = vtkm::dot(E23, P_prime);
      if (vtkm::Abs(det_prime) < vtkm::Epsilon<Precision>())
        return false;
      Precision inv_det_prime = 1.0f / det_prime;
      vec3 T_prime = ray_origin - v11;
      Precision alpha_prime = vtkm::dot(T_prime, P_prime) * inv_det_prime;
      if (alpha_prime < 0.0f)
        return false;
      vec3 Q_prime = vtkm::Cross(T_prime, E23);
      Precision beta_prime = vtkm::dot(ray_direction, Q_prime) * inv_det_prime;
      if (beta_prime < 0.0f)
        return false;
    }

    // Compute the ray parameter of the intersection point, and
    // reject the ray if it does not hit Q.

    t = vtkm::dot(E03, Q) * inv_det;
    if (t < 0.0)
      return false;


    // Compute the barycentric coordinates of V11
    Precision alpha_11, beta_11;
    vec3 E02 = v11 - v00;
    vec3 n = vtkm::Cross(E01, E02);

    if ((vtkm::Abs(n[0]) >= vtkm::Abs(n[1])) && (vtkm::Abs(n[0]) >= vtkm::Abs(n[2])))
    {

      alpha_11 = ((E02[1] * E03[2]) - (E02[2] * E03[1])) / n[0];
      beta_11 = ((E01[1] * E02[2]) - (E01[2] * E02[1])) / n[0];
    }
    else if ((vtkm::Abs(n[1]) >= vtkm::Abs(n[0])) && (vtkm::Abs(n[1]) >= vtkm::Abs(n[2])))
    {

      alpha_11 = ((E02[2] * E03[0]) - (E02[0] * E03[2])) / n[1];
      beta_11 = ((E01[2] * E02[0]) - (E01[0] * E02[2])) / n[1];
    }
    else
    {

      alpha_11 = ((E02[0] * E03[1]) - (E02[1] * E03[0])) / n[2];
      beta_11 = ((E01[0] * E02[1]) - (E01[1] * E02[0])) / n[2];
    }

    // Compute the bilinear coordinates of the intersection point.
    if (vtkm::Abs(alpha_11 - 1.0f) < vtkm::Epsilon<Precision>())
    {

      u = alpha;
      if (vtkm::Abs(beta_11 - 1.0f) < vtkm::Epsilon<Precision>())
        v = beta;
      else
        v = beta / ((u * (beta_11 - 1.0f)) + 1.0f);
    }
    else if (vtkm::Abs(beta_11 - 1.0) < vtkm::Epsilon<Precision>())
    {

      v = beta;
      u = alpha / ((v * (alpha_11 - 1.0f)) + 1.0f);
    }
    else
    {

      Precision A = 1.0f - beta_11;
      Precision B = (alpha * (beta_11 - 1.0f)) - (beta * (alpha_11 - 1.0f)) - 1.0f;
      Precision C = alpha;
      Precision D = (B * B) - (4.0f * A * C);
      Precision QQ = -0.5f * (B + ((B < 0.0f ? -1.0f : 1.0f) * vtkm::Sqrt(D)));
      u = QQ / A;
      if ((u < 0.0f) || (u > 1.0f))
        u = C / QQ;
      v = beta / ((u * (beta_11 - 1.0f)) + 1.0f);
    }

    return true;
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VTKM_EXEC inline void IntersectLeaf(
    const vtkm::Int32& currentNode,
    const vtkm::Vec<Precision, 3>& origin,
    const vtkm::Vec<Precision, 3>& dir,
    const PointPortalType& points,
    vtkm::Id& hitIndex,
    Precision& closestDistance, // closest distance in this set of primitives
    Precision& minU,
    Precision& minV,
    LeafPortalType leafs,
    const Precision& minDistance) const // report intesections past this distance
  {
    const vtkm::Id quadCount = leafs.Get(currentNode);
    for (vtkm::Id i = 1; i <= quadCount; ++i)
    {
      const vtkm::Id quadIndex = leafs.Get(currentNode + i);
      if (quadIndex < QuadIds.GetNumberOfValues())
      {
        IdType pointIndex = QuadIds.Get(quadIndex);
        Precision dist;
        vtkm::Vec<Precision, 3> q, r, s, t;
        q = vtkm::Vec<Precision, 3>(points.Get(pointIndex[1]));
        r = vtkm::Vec<Precision, 3>(points.Get(pointIndex[2]));
        s = vtkm::Vec<Precision, 3>(points.Get(pointIndex[3]));
        t = vtkm::Vec<Precision, 3>(points.Get(pointIndex[4]));
        Precision u, v;

        bool ret = quad(origin, dir, q, r, s, t, u, v, dist);
        if (ret)
        {
          if (dist < closestDistance && dist > minDistance)
          {
            //matid = vtkm::Vec<, 3>(points.Get(cur_offset + 2))[0];
            closestDistance = dist;
            hitIndex = quadIndex;
            minU = u;
            minV = v;
          }
        }
      }
    } // for
  }
};

class QuadExecWrapper : public vtkm::cont::ExecutionObjectBase
{
protected:
  using IdType = vtkm::Vec<vtkm::Id, 5>;
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>>;
  IdHandle QuadIds;

public:
  QuadExecWrapper(IdHandle& quadIds)
    : QuadIds(quadIds)
  {
  }

  template <typename Device>
  VTKM_CONT QuadLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return QuadLeafIntersector<Device>(QuadIds);
  }
};

class CalculateNormals : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CalculateNormals() {}
  typedef void
    ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut, FieldOut, WholeArrayIn, WholeArrayIn);
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

    vtkm::Vec<vtkm::Id, 5> quadId = indicesPortal.Get(hitIndex);

    vtkm::Vec<Precision, 3> a, b, c;
    a = points.Get(quadId[1]);
    b = points.Get(quadId[2]);
    c = points.Get(quadId[3]);

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
class GetScalar : public vtkm::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision invDeltaScalar;

public:
  VTKM_CONT
  GetScalar(const vtkm::Float32& minScalar, const vtkm::Float32& maxScalar)
    : MinScalar(minScalar)
  {
    //Make sure the we don't divide by zero on
    //something like an iso-surface
    if (maxScalar - MinScalar != 0.f)
      invDeltaScalar = 1.f / (maxScalar - MinScalar);
    else
      invDeltaScalar = 1.f / minScalar;
  }
  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename ScalarPortalType, typename IndicesPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                            Precision& scalar,
                            const ScalarPortalType& scalars,
                            const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    //TODO: this should be interpolated?
    vtkm::Vec<vtkm::Id, 5> pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId[0]));
    //normalize
    scalar = (scalar - MinScalar) * invDeltaScalar;
  }
}; //class GetScalar

} // namespace detail

QuadIntersector::QuadIntersector()
  : ShapeIntersector()
{
}

QuadIntersector::~QuadIntersector()
{
}


void QuadIntersector::IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void QuadIntersector::IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void QuadIntersector::IntersectRaysImp(Ray<Precision>& rays, bool vtkmNotUsed(returnCellIndex))
{

  detail::QuadExecWrapper leafIntersector(this->QuadIds);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void QuadIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                          const vtkm::cont::Field scalarField,
                                          const vtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  // TODO: if this is nodes of a mesh, support points
  const bool isSupportedField = scalarField.IsFieldCell() || scalarField.IsFieldPoint();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue("Field not accociated with a cell set");
  }

  vtkm::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, CoordsHandle, QuadIds);

  vtkm::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            scalarField.GetData().ResetTypes(vtkm::TypeListFieldScalar()),
            QuadIds);
}

void QuadIntersector::IntersectionData(Ray<vtkm::Float32>& rays,
                                       const vtkm::cont::Field scalarField,
                                       const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void QuadIntersector::IntersectionData(Ray<vtkm::Float64>& rays,
                                       const vtkm::cont::Field scalarField,
                                       const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void QuadIntersector::SetData(const vtkm::cont::CoordinateSystem& coords,
                              vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>> quadIds)
{

  this->QuadIds = quadIds;
  this->CoordsHandle = coords;
  AABBs AABB;

  vtkm::worklet::DispatcherMapField<detail::FindQuadAABBs> faabbsInvoker;
  faabbsInvoker.Invoke(this->QuadIds,
                       AABB.xmins,
                       AABB.ymins,
                       AABB.zmins,
                       AABB.xmaxs,
                       AABB.ymaxs,
                       AABB.zmaxs,
                       CoordsHandle);

  this->SetAABBs(AABB);
}

vtkm::Id QuadIntersector::GetNumberOfShapes() const
{
  return QuadIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
