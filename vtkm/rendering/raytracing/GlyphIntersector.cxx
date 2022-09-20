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
#include <vtkm/rendering/raytracing/GlyphIntersector.h>
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

class FindGlyphAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindGlyphAABBs() {}
  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pointId,
                            const vtkm::Float32& size,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    vtkm::Vec3f_32 point;
    point = static_cast<vtkm::Vec3f_32>(points.Get(pointId));
    vtkm::Float32 absSize = vtkm::Abs(size);

    xmin = point[0] - absSize;
    xmax = point[0] + absSize;
    ymin = point[1] - absSize;
    ymax = point[1] + absSize;
    zmin = point[2] - absSize;
    zmax = point[2] + absSize;
  }
}; //class FindGlyphAABBs

template <typename Device>
class GlyphLeafIntersector
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using IdArrayPortal = typename IdHandle::ReadPortalType;
  using FloatHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using VecHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>;
  using FloatPortal = typename FloatHandle::ReadPortalType;
  using VecPortal = typename VecHandle::ReadPortalType;
  IdArrayPortal PointIds;
  FloatPortal Sizes;
  vtkm::rendering::GlyphType GlyphType;

  GlyphLeafIntersector() {}

  GlyphLeafIntersector(const IdHandle& pointIds,
                       const FloatHandle& sizes,
                       vtkm::rendering::GlyphType glyphType,
                       vtkm::cont::Token& token)
    : PointIds(pointIds.PrepareForInput(Device(), token))
    , Sizes(sizes.PrepareForInput(Device(), token))
    , GlyphType(glyphType)
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
      Precision size = Sizes.Get(idx);
      vtkm::Vec<Precision, 3> point = vtkm::Vec<Precision, 3>(points.Get(pointIndex));

      if (this->GlyphType == vtkm::rendering::GlyphType::Sphere)
      {
        this->IntersectSphere(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
      else if (this->GlyphType == vtkm::rendering::GlyphType::Cube)
      {
        this->IntersectCube(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
      else if (this->GlyphType == vtkm::rendering::GlyphType::Axes)
      {
        this->IntersectAxes(
          origin, dir, point, size, pointIndex, hitIndex, closestDistance, minU, minV, minDistance);
      }
    }
  }

  template <typename Precision>
  VTKM_EXEC inline void IntersectSphere(const vtkm::Vec<Precision, 3>& origin,
                                        const vtkm::Vec<Precision, 3>& dir,
                                        const vtkm::Vec<Precision, 3>& point,
                                        const Precision& size,
                                        const vtkm::Id& pointIndex,
                                        vtkm::Id& hitIndex,
                                        Precision& closestDistance,
                                        Precision& vtkmNotUsed(minU),
                                        Precision& vtkmNotUsed(minV),
                                        const Precision& minDistance) const
  {
    vtkm::Vec<Precision, 3> l = point - origin;
    Precision dot1 = vtkm::dot(l, dir);
    if (dot1 >= 0)
    {
      Precision d = vtkm::dot(l, l) - dot1 * dot1;
      Precision r2 = size * size;
      if (d <= r2)
      {
        Precision tch = vtkm::Sqrt(r2 - d);
        Precision t0 = dot1 - tch;

        if (t0 < closestDistance && t0 > minDistance)
        {
          hitIndex = pointIndex;
          closestDistance = t0;
        }
      }
    }
  }

  template <typename Precision>
  VTKM_EXEC inline void IntersectCube(const vtkm::Vec<Precision, 3>& origin,
                                      const vtkm::Vec<Precision, 3>& dir,
                                      const vtkm ::Vec<Precision, 3>& point,
                                      const Precision& size,
                                      const vtkm::Id& pointIndex,
                                      vtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& vtkmNotUsed(minU),
                                      Precision& vtkmNotUsed(minV),
                                      const Precision& minDistance) const
  {
    Precision xmin, xmax, ymin, ymax, zmin, zmax;
    this->CalculateAABB(point, size, xmin, ymin, zmin, xmax, ymax, zmax);

    Precision tmin = (xmin - origin[0]) / dir[0];
    Precision tmax = (xmax - origin[0]) / dir[0];

    if (tmin > tmax)
      vtkm::Swap(tmin, tmax);

    Precision tymin = (ymin - origin[1]) / dir[1];
    Precision tymax = (ymax - origin[1]) / dir[1];
    if (tymin > tymax)
      vtkm::Swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
      return;

    if (tymin > tmin)
      tmin = tymin;

    if (tymax < tmax)
      tmax = tymax;

    Precision tzmin = (zmin - origin[2]) / dir[2];
    Precision tzmax = (zmax - origin[2]) / dir[2];

    if (tzmin > tzmax)
      vtkm::Swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
      return;

    if (tzmin > tmin)
      tmin = tzmin;

    if (tzmax < tmax)
      tmax = tzmax;

    if (tmin < closestDistance && tmin > minDistance)
    {
      hitIndex = pointIndex;
      closestDistance = tmin;
    }
  }

  template <typename Precision>
  VTKM_EXEC inline void IntersectAxes(const vtkm::Vec<Precision, 3>& origin,
                                      const vtkm::Vec<Precision, 3>& dir,
                                      const vtkm::Vec<Precision, 3>& point,
                                      const Precision& size,
                                      const vtkm::Id& pointIndex,
                                      vtkm::Id& hitIndex,
                                      Precision& closestDistance,
                                      Precision& vtkmNotUsed(minU),
                                      Precision& vtkmNotUsed(minV),
                                      const Precision& minDistance) const
  {
    Precision xmin, xmax, ymin, ymax, zmin, zmax;
    this->CalculateAABB(point, size, xmin, ymin, zmin, xmax, ymax, zmax);

    Precision t = (point[0] - origin[0]) / dir[0];
    vtkm::Vec<Precision, 3> intersection = origin + t * dir;

    if ((intersection[1] >= ymin && intersection[1] <= ymax) &&
        (intersection[2] >= zmin && intersection[2] <= zmax))
    {
      if (t < closestDistance && t > minDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
      }
    }

    t = (point[1] - origin[1]) / dir[1];
    intersection = origin + t * dir;
    if ((intersection[0] >= xmin && intersection[0] <= xmax) &&
        (intersection[2] >= zmin && intersection[2] <= zmax))
    {
      if (t < closestDistance && t > minDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
      }
    }

    t = (point[2] - origin[2]) / dir[2];
    intersection = origin + t * dir;
    if ((intersection[0] >= xmin && intersection[0] <= xmax) &&
        (intersection[1] >= ymin && intersection[1] <= ymax))
    {
      if (t < closestDistance && t > minDistance)
      {
        hitIndex = pointIndex;
        closestDistance = t;
      }
    }
  }

  template <typename Precision>
  VTKM_EXEC void CalculateAABB(const vtkm::Vec<Precision, 3>& point,
                               const Precision& size,
                               Precision& xmin,
                               Precision& ymin,
                               Precision& zmin,
                               Precision& xmax,
                               Precision& ymax,
                               Precision& zmax) const
  {
    Precision absSize = vtkm::Abs(size);
    xmin = point[0] - absSize;
    xmax = point[0] + absSize;
    ymin = point[1] - absSize;
    ymax = point[1] + absSize;
    zmin = point[2] - absSize;
    zmax = point[2] + absSize;
  }
};

class GlyphLeafWrapper : public vtkm::cont::ExecutionObjectBase
{
protected:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using FloatHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using VecHandle = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>;
  IdHandle PointIds;
  FloatHandle Sizes;
  vtkm::rendering::GlyphType GlyphType;

public:
  GlyphLeafWrapper(IdHandle& pointIds, FloatHandle sizes, vtkm::rendering::GlyphType glyphType)
    : PointIds(pointIds)
    , Sizes(sizes)
    , GlyphType(glyphType)
  {
  }

  template <typename Device>
  VTKM_CONT GlyphLeafIntersector<Device> PrepareForExecution(Device, vtkm::cont::Token& token) const
  {
    return GlyphLeafIntersector<Device>(this->PointIds, this->Sizes, this->GlyphType, token);
  }
}; // class GlyphLeafWrapper

class CalculateGlyphNormals : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CalculateGlyphNormals(vtkm::rendering::GlyphType glyphType)
    : GlyphType(glyphType)
  {
  }

  typedef void ControlSignature(FieldIn,
                                FieldIn,
                                FieldIn,
                                FieldOut,
                                FieldOut,
                                FieldOut,
                                WholeArrayIn,
                                WholeArrayIn,
                                WholeArrayIn);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename Precision,
            typename PointPortalType,
            typename IndicesPortalType,
            typename SizesPortalType>
  VTKM_EXEC inline void operator()(const vtkm::Id& hitIndex,
                                   const vtkm::Vec<Precision, 3>& rayDir,
                                   const vtkm::Vec<Precision, 3>& intersection,
                                   Precision& normalX,
                                   Precision& normalY,
                                   Precision& normalZ,
                                   const PointPortalType& points,
                                   const IndicesPortalType& indicesPortal,
                                   const SizesPortalType& sizesPortal) const
  {
    if (hitIndex < 0)
      return;

    vtkm::Id pointId = indicesPortal.Get(hitIndex);
    vtkm::Vec<Precision, 3> point = points.Get(pointId);
    Precision size = sizesPortal.Get(hitIndex);

    if (this->GlyphType == vtkm::rendering::GlyphType::Sphere)
    {
      this->CalculateNormalForSphere(rayDir, intersection, point, size, normalX, normalY, normalZ);
    }
    else if (this->GlyphType == vtkm::rendering::GlyphType::Cube)
    {
      this->CalculateNormalForCube(rayDir, intersection, point, size, normalX, normalY, normalZ);
    }
    else if (this->GlyphType == vtkm::rendering::GlyphType::Axes)
    {
      this->CalculateNormalForAxes(rayDir, intersection, point, size, normalX, normalY, normalZ);
    }
  }

  template <typename Precision>
  VTKM_EXEC inline void CalculateNormalForSphere(const vtkm::Vec<Precision, 3>& rayDir,
                                                 const vtkm::Vec<Precision, 3>& intersection,
                                                 const vtkm::Vec<Precision, 3>& point,
                                                 const Precision& vtkmNotUsed(size),
                                                 Precision& normalX,
                                                 Precision& normalY,
                                                 Precision& normalZ) const
  {
    vtkm::Vec<Precision, 3> normal = intersection - point;
    vtkm::Normalize(normal);

    // Flip normal if it is pointing the wrong way
    if (vtkm::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  template <typename Precision>
  VTKM_EXEC inline void CalculateNormalForCube(const vtkm::Vec<Precision, 3>& rayDir,
                                               const vtkm::Vec<Precision, 3>& intersection,
                                               const vtkm::Vec<Precision, 3>& point,
                                               const Precision& size,
                                               Precision& normalX,
                                               Precision& normalY,
                                               Precision& normalZ) const
  {
    vtkm::Vec<Precision, 3> lp = intersection - point;

    // Localize the intersection point to the surface of the cube.
    // One of the components will be 1 or -1 based on the face it lies on
    lp = lp * (1.0f / size);

    Precision eps = 1e-4f;
    vtkm::Vec<Precision, 3> normal{ 0.0f, 0.0f, 0.0f };
    normal[0] = (vtkm::Abs(vtkm::Abs(lp[0]) - 1.0f) <= eps) ? lp[0] : 0.0f;
    normal[1] = (vtkm::Abs(vtkm::Abs(lp[1]) - 1.0f) <= eps) ? lp[1] : 0.0f;
    normal[2] = (vtkm::Abs(vtkm::Abs(lp[2]) - 1.0f) <= eps) ? lp[2] : 0.0f;
    vtkm::Normalize(normal);

    // Flip normal if it is pointing the wrong way
    if (vtkm::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  template <typename Precision>
  VTKM_EXEC inline void CalculateNormalForAxes(const vtkm::Vec<Precision, 3>& rayDir,
                                               const vtkm::Vec<Precision, 3>& intersection,
                                               const vtkm::Vec<Precision, 3>& point,
                                               const Precision& vtkmNotUsed(size),
                                               Precision& normalX,
                                               Precision& normalY,
                                               Precision& normalZ) const
  {
    vtkm::Vec<Precision, 3> normal{ 0.0f, 0.0f, 0.0f };

    if (this->ApproxEquals(point[0], intersection[0]))
    {
      normal[0] = 1.0f;
    }
    else if (this->ApproxEquals(point[1], intersection[1]))
    {
      normal[1] = 1.0f;
    }
    else
    {
      normal[2] = 1.0f;
    }

    // Flip normal if it is pointing the wrong way
    if (vtkm::Dot(normal, rayDir) > 0.0f)
    {
      normal = -normal;
    }

    normalX = normal[0];
    normalY = normal[1];
    normalZ = normal[2];
  }

  template <typename Precision>
  VTKM_EXEC inline Precision ApproxEquals(Precision x, Precision y, Precision eps = 1e-5f) const
  {
    return vtkm::Abs(x - y) <= eps;
  }

  vtkm::rendering::GlyphType GlyphType;
}; //class CalculateGlyphNormals

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
  template <typename ScalarPortalType, typename IndicesPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                            Precision& scalar,
                            const ScalarPortalType& scalars,
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
}; //class GetScalars
}

GlyphIntersector::GlyphIntersector(vtkm::rendering::GlyphType glyphType)
  : ShapeIntersector()
{
  this->SetGlyphType(glyphType);
}

GlyphIntersector::~GlyphIntersector() {}

void GlyphIntersector::SetGlyphType(vtkm::rendering::GlyphType glyphType)
{
  this->GlyphType = glyphType;
}

void GlyphIntersector::SetData(const vtkm::cont::CoordinateSystem& coords,
                               vtkm::cont::ArrayHandle<vtkm::Id> pointIds,
                               vtkm::cont::ArrayHandle<vtkm::Float32> sizes)
{
  this->PointIds = pointIds;
  this->Sizes = sizes;
  this->CoordsHandle = coords;
  AABBs AABB;
  vtkm::worklet::DispatcherMapField<detail::FindGlyphAABBs>(detail::FindGlyphAABBs())
    .Invoke(PointIds,
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

void GlyphIntersector::IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

void GlyphIntersector::IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
void GlyphIntersector::IntersectRaysImp(Ray<Precision>& rays, bool vtkmNotUsed(returnCellIndex))
{
  detail::GlyphLeafWrapper leafIntersector(this->PointIds, Sizes, this->GlyphType);

  BVHTraverser traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void GlyphIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                           const vtkm::cont::Field scalarField,
                                           const vtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  const bool isSupportedField = scalarField.IsCellField() || scalarField.IsPointField();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue(
      "GlyphIntersector: Field not accociated with a cell set or field");
  }

  vtkm::worklet::DispatcherMapField<detail::CalculateGlyphNormals>(
    detail::CalculateGlyphNormals(this->GlyphType))
    .Invoke(rays.HitIdx,
            rays.Dir,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds,
            Sizes);

  vtkm::worklet::DispatcherMapField<detail::GetScalars<Precision>>(
    detail::GetScalars<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx,
            rays.Scalar,
            vtkm::rendering::raytracing::GetScalarFieldArray(scalarField),
            PointIds);
}

void GlyphIntersector::IntersectionData(Ray<vtkm::Float32>& rays,
                                        const vtkm::cont::Field scalarField,
                                        const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void GlyphIntersector::IntersectionData(Ray<vtkm::Float64>& rays,
                                        const vtkm::cont::Field scalarField,
                                        const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

vtkm::Id GlyphIntersector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
