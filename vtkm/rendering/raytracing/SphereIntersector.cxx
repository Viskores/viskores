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

#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/rendering/raytracing/BVHTraverser.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/SphereIntersector.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class FindSphereAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindSphereAABBs() {}
  typedef void ControlSignature(FieldIn<>,
                                FieldIn<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                FieldOut<>,
                                WholeArrayIn<Vec3RenderingTypes>);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, _8, _9);
  template <typename PointPortalType>
  VTKM_EXEC void operator()(const vtkm::Id pointId,
                            const vtkm::Float32& radius,
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
    vtkm::Vec<vtkm::Float32, 3> temp;
    point = static_cast<vtkm::Vec<vtkm::Float32, 3>>(points.Get(pointId));

    temp[0] = radius;
    temp[1] = 0.f;
    temp[2] = 0.f;

    vtkm::Vec<vtkm::Float32, 3> p = point + temp;
    //set first point to max and min
    xmin = p[0];
    xmax = p[0];
    ymin = p[1];
    ymax = p[1];
    zmin = p[2];
    zmax = p[2];

    p = point - temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = radius;
    temp[2] = 0.f;

    p = point + temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    temp[0] = 0.f;
    temp[1] = 0.f;
    temp[2] = radius;

    p = point + temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);

    p = point - temp;
    xmin = vtkm::Min(xmin, p[0]);
    xmax = vtkm::Max(xmax, p[0]);
    ymin = vtkm::Min(ymin, p[1]);
    ymax = vtkm::Max(ymax, p[1]);
    zmin = vtkm::Min(zmin, p[2]);
    zmax = vtkm::Max(zmax, p[2]);
  }
}; //class FindAABBs

template <typename Device>
class SphereLeafIntersector
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using IdArrayPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using FloatHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using FloatPortal = typename FloatHandle::ExecutionTypes<Device>::PortalConst;
  IdArrayPortal PointIds;
  FloatPortal Radii;

  SphereLeafIntersector(const IdHandle& pointIds, const FloatHandle& radii)
    : PointIds(pointIds.PrepareForInput(Device()))
    , Radii(radii.PrepareForInput(Device()))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VTKM_EXEC inline void IntersectLeaf(
    const vtkm::Int32& currentNode,
    const vtkm::Vec<Precision, 3>& origin,
    const vtkm::Vec<Precision, 3>& dir,
    const PointPortalType& points,
    vtkm::Id& hitIndex,
    Precision& closestDistance, // closest distance in this set of primitives
    Precision& vtkmNotUsed(minU),
    Precision& vtkmNotUsed(minV),
    LeafPortalType leafs,
    const Precision& minDistance) const // report intesections past this distance
  {
    const vtkm::Id sphereCount = leafs.Get(currentNode);
    for (vtkm::Id i = 1; i <= sphereCount; ++i)
    {
      const vtkm::Id sphereIndex = leafs.Get(currentNode + i);
      vtkm::Id pointIndex = PointIds.Get(sphereIndex);
      vtkm::Float32 radius = Radii.Get(sphereIndex);
      vtkm::Vec<Precision, 3> center = vtkm::Vec<Precision, 3>(points.Get(pointIndex));

      vtkm::Vec<Precision, 3> l = center - origin;

      Precision dot1 = vtkm::dot(l, dir);

      if (dot1 >= 0)
      {
        Precision d = vtkm::dot(l, l) - dot1 * dot1;
        Precision r2 = radius * radius;
        if (d <= r2)
        {
          Precision tch = vtkm::Sqrt(r2 - d);
          Precision t0 = dot1 - tch;
          //float t1 = dot1+tch; /* if t1 is > 0 and t0<0 then the ray is inside the sphere.

          if (t0 < closestDistance && t0 > minDistance)
          {
            hitIndex = pointIndex;
            closestDistance = t0;
          }
        }
      }
    } // for
  }
};

struct IntersectFunctor
{
  template <typename Device, typename Precision>
  VTKM_CONT bool operator()(Device,
                            SphereIntersector* self,
                            Ray<Precision>& rays,
                            bool returnCellIndex)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    self->IntersectRaysImp(Device(), rays, returnCellIndex);
    return true;
  }
};

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
                                   const vtkm::Vec<Precision, 3>& intersection,
                                   Precision& normalX,
                                   Precision& normalY,
                                   Precision& normalZ,
                                   const PointPortalType& points,
                                   const IndicesPortalType& indicesPortal) const
  {
    if (hitIndex < 0)
      return;

    vtkm::Id pointId = indicesPortal.Get(hitIndex);
    vtkm::Vec<Precision, 3> center = points.Get(pointId);

    vtkm::Vec<Precision, 3> normal = intersection - center;
    vtkm::Normalize(normal);

    //flip the normal if its pointing the wrong way
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
  typedef void ControlSignature(FieldIn<>,
                                FieldInOut<>,
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

    vtkm::Id pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId));
    //normalize
    scalar = (scalar - MinScalar) * invDeltaScalar;
  }
}; //class GetScalar

} // namespace detail

SphereIntersector::SphereIntersector()
  : ShapeIntersector()
{
}

SphereIntersector::~SphereIntersector()
{
}

void SphereIntersector::SetData(const vtkm::cont::CoordinateSystem& coords,
                                vtkm::cont::ArrayHandle<vtkm::Id> pointIds,
                                vtkm::cont::ArrayHandle<vtkm::Float32> radii)
{
  this->PointIds = pointIds;
  this->Radii = radii;
  this->CoordsHandle = coords;
  AABBs AABB;
  vtkm::worklet::DispatcherMapField<detail::FindSphereAABBs>(detail::FindSphereAABBs())
    .Invoke(PointIds,
            Radii,
            AABB.xmins,
            AABB.ymins,
            AABB.zmins,
            AABB.xmaxs,
            AABB.ymaxs,
            AABB.zmaxs,
            CoordsHandle);

  this->SetAABBs(AABB);
}

void SphereIntersector::IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex)
{
  vtkm::cont::TryExecute(detail::IntersectFunctor(), this, rays, returnCellIndex);
}

void SphereIntersector::IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex)
{
  vtkm::cont::TryExecute(detail::IntersectFunctor(), this, rays, returnCellIndex);
}

template <typename Device, typename Precision>
void SphereIntersector::IntersectRaysImp(Device,
                                         Ray<Precision>& rays,
                                         bool vtkmNotUsed(returnCellIndex))
{

  detail::SphereLeafIntersector<Device> leafIntersector(this->PointIds, Radii);

  BVHTraverser<detail::SphereLeafIntersector> traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle, Device());

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void SphereIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                            const vtkm::cont::Field* scalarField,
                                            const vtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  bool isSupportedField =
    (scalarField->GetAssociation() == vtkm::cont::Field::Association::POINTS ||
     scalarField->GetAssociation() == vtkm::cont::Field::Association::CELL_SET);
  if (!isSupportedField)
    throw vtkm::cont::ErrorBadValue(
      "SphereIntersector: Field not accociated with a cell set or field");

  vtkm::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            PointIds);

  vtkm::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx, rays.Scalar, *scalarField, PointIds);
}

void SphereIntersector::IntersectionData(Ray<vtkm::Float32>& rays,
                                         const vtkm::cont::Field* scalarField,
                                         const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void SphereIntersector::IntersectionData(Ray<vtkm::Float64>& rays,
                                         const vtkm::cont::Field* scalarField,
                                         const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

vtkm::Id SphereIntersector::GetNumberOfShapes() const
{
  return PointIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
