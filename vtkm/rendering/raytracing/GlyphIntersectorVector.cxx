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
#include <vtkm/rendering/raytracing/GlyphIntersectorVector.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

// This line is at the end to prevent warnings when building for CUDA
#include <vtkm/Swap.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{
static constexpr vtkm::Float32 ARROW_BODY_SIZE = 0.75f;

class FindGlyphVectorAABBs : public vtkm::worklet::WorkletMapField
{
  vtkm::rendering::GlyphType GlyphType;
  vtkm::Float32 ArrowBodyRadius;
  vtkm::Float32 ArrowHeadRadius;

public:
  using ControlSignature = void(FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  VTKM_CONT
  FindGlyphVectorAABBs(vtkm::rendering::GlyphType glyphType,
                       vtkm::Float32 bodyRadius,
                       vtkm::Float32 headRadius)
    : GlyphType(glyphType)
    , ArrowBodyRadius(bodyRadius)
    , ArrowHeadRadius(headRadius)
  {
  }

  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            const vtkm::Vec3f_32& size,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    vtkm::Vec3f_32 point = static_cast<vtkm::Vec3f_32>(points.Get(pointId));
    xmin = point[0];
    xmax = point[0];
    ymin = point[1];
    ymax = point[1];
    zmin = point[2];
    zmax = point[2];

    if (this->GlyphType == vtkm::rendering::GlyphType::Arrow)
    {
      this->CalculateArrowAABB(point, size, xmin, ymin, zmin, xmax, ymax, zmax);
    }
  }

  VTKM_EXEC inline void CalculateArrowAABB(const vtkm::Vec3f_32& point,
                                           const vtkm::Vec3f_32& size,
                                           vtkm::Float32& xmin,
                                           vtkm::Float32& ymin,
                                           vtkm::Float32& zmin,
                                           vtkm::Float32& xmax,
                                           vtkm::Float32& ymax,
                                           vtkm::Float32& zmax) const
  {
    vtkm::Vec3f_32 body_pa = point;
    vtkm::Vec3f_32 body_pb = body_pa + ARROW_BODY_SIZE * size;
    vtkm::Vec3f_32 head_pa = body_pb;
    vtkm::Vec3f_32 head_pb = point + size;

    this->CylinderAABB(body_pa, body_pb, this->ArrowBodyRadius, xmin, ymin, zmin, xmax, ymax, zmax);
    this->ConeAABB(
      head_pa, head_pb, this->ArrowHeadRadius, 0.0f, xmin, ymin, zmin, xmax, ymax, zmax);
  }

  VTKM_EXEC inline void CylinderAABB(const vtkm::Vec3f_32& pa,
                                     const vtkm::Vec3f_32& pb,
                                     const vtkm::Float32& ra,
                                     vtkm::Float32& xmin,
                                     vtkm::Float32& ymin,
                                     vtkm::Float32& zmin,
                                     vtkm::Float32& xmax,
                                     vtkm::Float32& ymax,
                                     vtkm::Float32& zmax) const
  {
    vtkm::Vec3f_32 a = pb - pa;
    vtkm::Vec3f_32 e_prime = a * a / vtkm::Dot(a, a);
    vtkm::Vec3f_32 e = ra * vtkm::Sqrt(1.0f - e_prime);

    vtkm::Vec3f_32 pa1 = pa - e;
    vtkm::Vec3f_32 pa2 = pa + e;
    vtkm::Vec3f_32 pb1 = pb - e;
    vtkm::Vec3f_32 pb2 = pb + e;

    xmin = vtkm::Min(xmin, vtkm::Min(pa1[0], pb1[0]));
    ymin = vtkm::Min(ymin, vtkm::Min(pa1[1], pb1[1]));
    zmin = vtkm::Min(zmin, vtkm::Min(pa1[2], pb1[2]));
    xmax = vtkm::Max(xmax, vtkm::Max(pa2[0], pb2[0]));
    ymax = vtkm::Max(ymax, vtkm::Max(pa2[1], pb2[1]));
    zmax = vtkm::Max(zmax, vtkm::Max(pa2[2], pb2[2]));
  }

  VTKM_EXEC inline void ConeAABB(const vtkm::Vec3f_32& pa,
                                 const vtkm::Vec3f_32& pb,
                                 const vtkm::Float32& ra,
                                 const vtkm::Float32& rb,
                                 vtkm::Float32& xmin,
                                 vtkm::Float32& ymin,
                                 vtkm::Float32& zmin,
                                 vtkm::Float32& xmax,
                                 vtkm::Float32& ymax,
                                 vtkm::Float32& zmax) const
  {
    vtkm::Vec3f_32 a = pb - pa;
    vtkm::Vec3f_32 e_prime = a * a / vtkm::Dot(a, a);
    vtkm::Vec3f_32 e = vtkm::Sqrt(1.0f - e_prime);

    vtkm::Vec3f_32 pa1 = pa - e * ra;
    vtkm::Vec3f_32 pa2 = pa + e * ra;
    vtkm::Vec3f_32 pb1 = pb - e * rb;
    vtkm::Vec3f_32 pb2 = pb + e * rb;

    xmin = vtkm::Min(xmin, vtkm::Min(pa1[0], pb1[0]));
    ymin = vtkm::Min(ymin, vtkm::Min(pa1[1], pb1[1]));
    zmin = vtkm::Min(zmin, vtkm::Min(pa1[2], pb1[2]));
    xmax = vtkm::Max(xmax, vtkm::Max(pa2[0], pb2[0]));
    ymax = vtkm::Max(ymax, vtkm::Max(pa2[1], pb2[1]));
    zmax = vtkm::Max(zmax, vtkm::Max(pa2[2], pb2[2]));
  }
}; //class FindGlyphVectorAABBs

template <typename Device>
class GlyphVectorLeafIntersector
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  using Vec3f_32Handle = vtkm::cont::ArrayHandle<vtkm::Vec3f_32>;
  using Vec3f_32Portal = typename Vec3f_32Handle::ReadPortalType;

  vtkm::rendering::GlyphType GlyphType;
  IdArrayPortal PointIds;
  Vec3f_32Portal Sizes;
  vtkm::Float32 ArrowBodyRadius;
  vtkm::Float32 ArrowHeadRadius;

  GlyphVectorLeafIntersector() = default;

  GlyphVectorLeafIntersector(vtkm::rendering::GlyphType glyphType,
                             const IdHandle& pointIds,
                             const Vec3f_32Handle& sizes,
                             vtkm::Float32 bodyRadius,
                             vtkm::Float32 headRadius,
                             vtkm::cont::Token& token)
    : GlyphType(glyphType)
    , PointIds(pointIds.PrepareForInput(Device(), token))
    , Sizes(sizes.PrepareForInput(Device(), token))
    , ArrowBodyRadius(bodyRadius)
    , ArrowHeadRadius(headRadius)
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
    const vtkm::Id glyphCount = leafs.Get(currentNode);

    for (vtkm::Id i = 1; i <= glyphCount; ++i)
    {
      const vtkm::Id idx = leafs.Get(currentNode + i);
      vtkm::Id pointIndex = PointIds.Get(idx);
      vtkm::Vec<Precision, 3> size = Sizes.Get(idx);
      vtkm::Vec<Precision, 3> point = vtkm::Vec<Precision, 3>(points.Get(pointIndex));

      if (this->GlyphType == vtkm::rendering::GlyphType::Arrow)
      {
        this->IntersectArrow(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
    }
  }

  template <typename Precision>
  VTKM_EXEC inline void IntersectArrow(const vtkm::Vec<Precision, 3>& origin,
                                       const vtkm::Vec<Precision, 3>& dir,
                                       const vtkm::Vec<Precision, 3>& point,
                                       const vtkm::Vec<Precision, 3>& size,
                                       const vtkm::Id& pointIndex,
                                       vtkm::Id& hitIndex,
                                       Precision& closestDistance,
                                       Precision& minU,
                                       Precision& minV,
                                       const Precision& minDistance) const
  {
    using Vec2 = vtkm::Vec<Precision, 2>;
    using Vec3 = vtkm::Vec<Precision, 3>;
    using Vec4 = vtkm::Vec<Precision, 4>;

    Vec3 body_pa = point;
    Vec3 body_pb = body_pa + ARROW_BODY_SIZE * size;
    Vec3 head_pa = body_pb;
    Vec3 head_pb = point + size;

    Vec4 bodyIntersection =
      this->IntersectCylinder(origin, dir, body_pa, body_pb, Precision(this->ArrowBodyRadius));
    Vec4 headIntersection = this->IntersectCone(
      origin, dir, head_pa, head_pb, Precision(this->ArrowHeadRadius), Precision(0.0f));

    bool bodyHit = bodyIntersection[0] >= minDistance;
    bool headHit = headIntersection[0] >= minDistance;
    if (bodyHit && !headHit)
    {
      Precision t = bodyIntersection[0];
      if (t < closestDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
        minU = bodyIntersection[1];
        minV = bodyIntersection[2];
      }
    }
    else if (!bodyHit && headHit)
    {
      Precision t = headIntersection[0];
      if (t < closestDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
        minU = headIntersection[1];
        minV = headIntersection[2];
      }
    }
    else if (bodyHit || headHit)
    {
      Precision t1 = bodyIntersection[0];
      Precision t2 = headIntersection[0];

      Precision t = t1;
      Vec2 partialNormal = { bodyIntersection[1], bodyIntersection[2] };
      if (t2 < t)
      {
        t = t2;
        partialNormal[0] = headIntersection[1];
        partialNormal[1] = headIntersection[2];
      }

      if (t < closestDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
        minU = partialNormal[0];
        minV = partialNormal[1];
      }
    }
  }

  template <typename Precision>
  VTKM_EXEC vtkm::Vec4f_32 IntersectCylinder(const vtkm::Vec<Precision, 3>& ro,
                                             const vtkm::Vec<Precision, 3>& rd,
                                             const vtkm::Vec<Precision, 3>& pa,
                                             const vtkm::Vec<Precision, 3>& pb,
                                             const Precision& ra) const
  {
    using Vec3 = vtkm::Vec<Precision, 3>;
    using Vec4 = vtkm::Vec<Precision, 4>;

    const Vec4 NO_HIT{ -1.0f, -1.0f, -1.0f, -1.0f };

    Vec3 cc = 0.5f * (pa + pb);
    Precision ch = vtkm::Magnitude(pb - pa);
    Vec3 ca = (pb - pa) / ch;
    ch *= 0.5f;

    Vec3 oc = ro - cc;

    Precision card = vtkm::Dot(ca, rd);
    Precision caoc = vtkm::Dot(ca, oc);

    Precision a = 1.0f - card * card;
    Precision b = vtkm::Dot(oc, rd) - caoc * card;
    Precision c = vtkm::Dot(oc, oc) - caoc * caoc - ra * ra;
    Precision h = b * b - a * c;
    if (h < 0.0f)
    {
      return NO_HIT;
    }

    h = vtkm::Sqrt(h);
    Precision t1 = (-b - h) / a;
    /* Precision t2 = (-b + h) / a; // exit point */

    Precision y = caoc + t1 * card;

    // body
    if (vtkm::Abs(y) < ch)
    {
      vtkm::Vec3f_32 normal = vtkm::Normal(oc + t1 * rd - ca * y);
      return vtkm::Vec4f_32(static_cast<vtkm::Float32>(t1), normal[0], normal[1], normal[2]);
    }

    // bottom cap
    Precision sy = -1;
    Precision tp = (sy * ch - caoc) / card;
    if (vtkm::Abs(b + a * tp) < h)
    {
      vtkm::Vec3f_32 normal = vtkm::Normal(ca * sy);
      return vtkm::Vec4f_32(static_cast<vtkm::Float32>(tp), normal[0], normal[1], normal[2]);
    }

    // top cap
    sy = 1;
    tp = (sy * ch - caoc) / card;
    if (vtkm::Abs(b + a * tp) < h)
    {
      vtkm::Vec3f_32 normal = vtkm::Normal(ca * sy);
      return vtkm::Vec4f_32(static_cast<vtkm::Float32>(tp), normal[0], normal[1], normal[2]);
    }

    return NO_HIT;
  }

  template <typename Precision>
  VTKM_EXEC vtkm::Vec4f_32 IntersectCone(const vtkm::Vec<Precision, 3>& ro,
                                         const vtkm::Vec<Precision, 3>& rd,
                                         const vtkm::Vec<Precision, 3>& pa,
                                         const vtkm::Vec<Precision, 3>& pb,
                                         const Precision& ra,
                                         const Precision& rb) const
  {
    using Vec3 = vtkm::Vec<Precision, 3>;
    using Vec4 = vtkm::Vec<Precision, 4>;

    const Vec4 NO_HIT{ -1.0f, -1.0f, -1.0f, -1.0f };

    Vec3 ba = pb - pa;
    Vec3 oa = ro - pa;
    Vec3 ob = ro - pb;

    Precision m0 = vtkm::Dot(ba, ba);
    Precision m1 = vtkm::Dot(oa, ba);
    Precision m2 = vtkm::Dot(ob, ba);
    Precision m3 = vtkm::Dot(rd, ba);

    //caps
    if (m1 < 0.0)
    {
      Vec3 m11 = oa * m3 - rd * m1;
      Precision m12 = ra * ra * m3 * m3;
      if (vtkm::Dot(m11, m11) < m12)
      {
        Precision t = -m1 / m3;
        Vec3 normal = -ba * 1.0f / vtkm::Sqrt(m0);
        return Vec4(t, normal[0], normal[1], normal[2]);
      }
    }
    else if (m2 > 0.0)
    {
      Vec3 m21 = ob * m3 - rd * m2;
      Precision m22 = rb * rb * m3 * m3;
      if (vtkm::Dot(m21, m21) < m22)
      {
        Precision t = -m2 / m3;
        Vec3 normal = ba * 1.0f / vtkm::Sqrt(m0);
        return Vec4(t, normal[0], normal[1], normal[2]);
      }
    }

    // body
    Precision rr = ra - rb;
    Precision hy = m0 + rr * rr;
    Precision m4 = vtkm::Dot(rd, oa);
    Precision m5 = vtkm::Dot(oa, oa);

    Precision k2 = m0 * m0 - m3 * m3 * hy;
    Precision k1 = m0 * m0 * m4 - m1 * m3 * hy + m0 * ra * (rr * m3 * 1.0f);
    Precision k0 = m0 * m0 * m5 - m1 * m1 * hy + m0 * ra * (rr * m1 * 2.0f - m0 * ra);

    Precision h = k1 * k1 - k2 * k0;
    if (h < 0.0)
    {
      return NO_HIT;
    }

    Precision t = (-k1 - sqrt(h)) / k2;
    Precision y = m1 + t * m3;

    if (y > 0.0 && y < m0)
    {
      Vec3 normal = vtkm::Normal(m0 * (m0 * (oa + t * rd) + rr * ba * ra) - ba * hy * y);
      return Vec4(t, normal[0], normal[1], normal[2]);
    }

    return NO_HIT;
  }
};

class GlyphVectorLeafWrapper : public vtkm::cont::ExecutionObjectBase
{
protected:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using Vec3f_32Handle = vtkm::cont::ArrayHandle<vtkm::Vec3f_32>;
  vtkm::rendering::GlyphType GlyphType;
  IdHandle PointIds;
  Vec3f_32Handle Sizes;
  vtkm::Float32 ArrowBodyRadius;
  vtkm::Float32 ArrowHeadRadius;

public:
  GlyphVectorLeafWrapper(vtkm::rendering::GlyphType glyphType,
                         IdHandle& pointIds,
                         Vec3f_32Handle& sizes,
                         vtkm::Float32 bodyRadius,
                         vtkm::Float32 headRadius)
    : GlyphType(glyphType)
    , PointIds(pointIds)
    , Sizes(sizes)
    , ArrowBodyRadius(bodyRadius)
    , ArrowHeadRadius(headRadius)
  {
  }

  template <typename Device>
  VTKM_CONT GlyphVectorLeafIntersector<Device> PrepareForExecution(Device,
                                                                   vtkm::cont::Token& token) const
  {
    return GlyphVectorLeafIntersector<Device>(this->GlyphType,
                                              this->PointIds,
                                              this->Sizes,
                                              this->ArrowBodyRadius,
                                              this->ArrowHeadRadius,
                                              token);
  }
};

class CalculateGlyphVectorNormals : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CalculateGlyphVectorNormals(vtkm::rendering::GlyphType glyphType)
    : GlyphType(glyphType)
  {
  }

  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11);

  template <typename Precision,
            typename PointPortalType,
            typename IndicesPortalType,
            typename SizesPortalType>
  VTKM_EXEC inline void operator()(const vtkm::Id& hitIndex,
                                   const vtkm::Vec<Precision, 3>& rayDir,
                                   const vtkm::Vec<Precision, 3>& intersection,
                                   const Precision& u,
                                   const Precision& v,
                                   Precision& normalX,
                                   Precision& normalY,
                                   Precision& normalZ,
                                   const PointPortalType& vtkmNotUsed(points),
                                   const IndicesPortalType& vtkmNotUsed(indicesPortal),
                                   const SizesPortalType& vtkmNotUsed(sizesPortal)) const
  {
    if (hitIndex < 0)
      return;

    if (this->GlyphType == vtkm::rendering::GlyphType::Arrow)
    {
      this->CalculateArrowNormal(rayDir, intersection, u, v, normalX, normalY, normalZ);
    }
  }

  template <typename Precision>
  VTKM_EXEC inline void CalculateArrowNormal(
    const vtkm::Vec<Precision, 3>& rayDir,
    const vtkm::Vec<Precision, 3>& vtkmNotUsed(intersection),
    const Precision& u,
    const Precision& v,
    Precision& normalX,
    Precision& normalY,
    Precision& normalZ) const
  {
    vtkm::Vec<Precision, 3> normal;
    normal[0] = u;
    normal[1] = v;
    normal[2] = 1.0f - (normalX * normalX) - (normalY * normalY);

    if (vtkm::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  vtkm::rendering::GlyphType GlyphType;
}; //class CalculateGlyphVectorNormals

template <typename Precision>
class GetScalars : public vtkm::worklet::WorkletMapField
{
private:
  Precision MinScalar;
  Precision InvDeltaScalar;
  bool Normalize;

public:
  VTKM_CONT
  GetScalars(const vtkm::Float32& minScalar, const vtkm::Float32& maxScalar)
    : MinScalar(minScalar)
  {
    Normalize = true;
    if (minScalar >= maxScalar)
    {
      // support the scalar renderer
      Normalize = false;
      this->InvDeltaScalar = Precision(0.f);
    }
    else
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      this->InvDeltaScalar = 1.f / (maxScalar - this->MinScalar);
    }
  }
  typedef void ControlSignature(FieldIn, FieldOut, WholeArrayIn, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  template <typename FieldPortalType, typename IndicesPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                            Precision& scalar,
                            const FieldPortalType& scalars,
                            const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    vtkm::Id pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId));
    if (Normalize)
    {
      scalar = (scalar - this->MinScalar) * this->InvDeltaScalar;
    }
  }
}; //class GetScalar

} // namespace

GlyphIntersectorVector::GlyphIntersectorVector(vtkm::rendering::GlyphType glyphType)
  : ShapeIntersector()
  , ArrowBodyRadius(0.004f)
  , ArrowHeadRadius(0.008f)
{
  this->SetGlyphType(glyphType);
}

GlyphIntersectorVector::~GlyphIntersectorVector() {}

void GlyphIntersectorVector::SetGlyphType(vtkm::rendering::GlyphType glyphType)
{
  this->GlyphType = glyphType;
}

void GlyphIntersectorVector::SetData(const vtkm::cont::CoordinateSystem& coords,
                                     vtkm::cont::ArrayHandle<vtkm::Id> pointIds,
                                     vtkm::cont::ArrayHandle<vtkm::Vec3f_32> sizes)
{
  this->PointIds = pointIds;
  this->Sizes = sizes;
  this->CoordsHandle = coords;
  AABBs AABB;
  vtkm::cont::Invoker invoker;
  invoker(
    detail::FindGlyphVectorAABBs{ this->GlyphType, this->ArrowBodyRadius, this->ArrowHeadRadius },
    PointIds,
    Sizes,
    AABB.xmins,
    AABB.ymins,
    AABB.zmins,
    AABB.xmaxs,
    AABB.ymaxs,
    AABB.zmaxs,
    CoordsHandle);

  this->SetAABBs(AABB);
}

void GlyphIntersectorVector::IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void GlyphIntersectorVector::IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void GlyphIntersectorVector::IntersectRaysImp(Ray<Precision>& rays,
                                              bool vtkmNotUsed(returnCellIndex))
{
  detail::GlyphVectorLeafWrapper leafIntersector(
    this->GlyphType, this->PointIds, this->Sizes, this->ArrowBodyRadius, this->ArrowHeadRadius);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void GlyphIntersectorVector::IntersectionDataImp(Ray<Precision>& rays,
                                                 const vtkm::cont::Field field,
                                                 const vtkm::Range& range)
{
  ShapeIntersector::IntersectionPoint(rays);

  const bool isSupportedField = field.IsCellField() || field.IsPointField();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue(
      "GlyphIntersectorVector: Field not accociated with a cell set or field");
  }

  vtkm::worklet::DispatcherMapField<detail::CalculateGlyphVectorNormals>(
    detail::CalculateGlyphVectorNormals(this->GlyphType))
    .Invoke(rays.HitIdx,
            rays.Dir,
            rays.Intersection,
            rays.U,
            rays.V,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds,
            Sizes);

  vtkm::worklet::DispatcherMapField<detail::GetScalars<Precision>>(
    detail::GetScalars<Precision>(vtkm::Float32(range.Min), vtkm::Float32(range.Max)))
    .Invoke(
      rays.HitIdx, rays.Scalar, vtkm::rendering::raytracing::GetScalarFieldArray(field), PointIds);
}

void GlyphIntersectorVector::IntersectionData(Ray<vtkm::Float32>& rays,
                                              const vtkm::cont::Field field,
                                              const vtkm::Range& range)
{
  IntersectionDataImp(rays, field, range);
}

void GlyphIntersectorVector::IntersectionData(Ray<vtkm::Float64>& rays,
                                              const vtkm::cont::Field field,
                                              const vtkm::Range& range)
{
  IntersectionDataImp(rays, field, range);
}

vtkm::Id GlyphIntersectorVector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}

void GlyphIntersectorVector::SetArrowRadii(vtkm::Float32 bodyRadius, vtkm::Float32 headRadius)
{
  this->ArrowHeadRadius = headRadius;
  this->ArrowBodyRadius = bodyRadius;
}

}
}
} //namespace vtkm::rendering::raytracing
