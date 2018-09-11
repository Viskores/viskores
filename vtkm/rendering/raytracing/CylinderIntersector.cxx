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
#include <vtkm/rendering/raytracing/CylinderIntersector.h>
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

template <typename Device>
class CylinderLeafIntersector : public vtkm::cont::ExecutionObjectBase
{
public:
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id3>;
  using IdArrayPortal = typename IdHandle::ExecutionTypes<Device>::PortalConst;
  using FloatHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using FloatPortal = typename FloatHandle::ExecutionTypes<Device>::PortalConst;
  IdArrayPortal CylIds;
  FloatPortal Radii;

  CylinderLeafIntersector(const IdHandle& cylIds, const FloatHandle& radii)
    : CylIds(cylIds.PrepareForInput(Device()))
    , Radii(radii.PrepareForInput(Device()))
  {
  }

  template <typename vec3>
  VTKM_EXEC vec3 cylinder(const vec3& ray_start,
                          const vec3& ray_direction,
                          const vec3& p,
                          const vec3& q,
                          float r) const
  {
    float t = 0;
    vec3 d = q - p;
    vec3 m = ray_start - p;

    vec3 s = ray_start - q;

    vtkm::Float32 mdotm = vtkm::Float32(vtkm::dot(m, m));
    vec3 n = ray_direction * (vtkm::Max(mdotm, static_cast<vtkm::Float32>(vtkm::dot(s, s))) + r);

    vtkm::Float32 mdotd = vtkm::Float32(vtkm::dot(m, d));
    vtkm::Float32 ndotd = vtkm::Float32(vtkm::dot(n, d));
    vtkm::Float32 ddotd = vtkm::Float32(vtkm::dot(d, d));
    if ((mdotd < 0.0f) && (mdotd + ndotd < 0.0f))
    {
      return vec3(0.f, 0.f, 0.f);
    }
    if ((mdotd > ddotd) && (mdotd + ndotd > ddotd))
    {
      return vec3(0.f, 0.f, 0.f);
    }
    vtkm::Float32 ndotn = vtkm::Float32(vtkm::dot(n, n));
    vtkm::Float32 nlen = vtkm::Float32(sqrt(ndotn));
    vtkm::Float32 mdotn = vtkm::Float32(vtkm::dot(m, n));
    vtkm::Float32 a = ddotd * ndotn - ndotd * ndotd;
    vtkm::Float32 k = mdotm - r * r;
    vtkm::Float32 c = ddotd * k - mdotd * mdotd;

    if (fabs(a) < 1e-6)
    {
      if (c > 0.0)
      {
        return vec3(0, 0, 0);
      }
      if (mdotd < 0.0f)
      {
        t = -mdotn / ndotn;
      }
      else if (mdotd > ddotd)
      {
        t = (ndotd - mdotn) / ndotn;
      }
      else
        t = 0;

      return vec3(1, t * nlen, 0);
    }
    vtkm::Float32 b = ddotd * mdotn - ndotd * mdotd;
    vtkm::Float32 discr = b * b - a * c;
    if (discr < 0.0f)
    {
      return vec3(0, 0, 0);
    }
    t = (-b - vtkm::Sqrt(discr)) / a;
    if (t < 0.0f || t > 1.0f)
    {
      return vec3(0, 0, 0);
    }

    vtkm::Float32 u = mdotd + t * ndotd;

    if (u > ddotd)
    {
      if (ndotd >= 0.0f)
      {
        return vec3(0, 0, 0);
      }
      t = (ddotd - mdotd) / ndotd;

      return vec3(
        k + ddotd - 2 * mdotd + t * (2 * (mdotn - ndotd) + t * ndotn) <= 0.0f, t * nlen, 0);
    }
    else if (u < 0.0f)
    {
      if (ndotd <= 0.0f)
      {
        return vec3(0.0, 0.0, 0);
      }
      t = -mdotd / ndotd;

      return vec3(k + 2 * t * (mdotn + t * ndotn) <= 0.0f, t * nlen, 0);
    }
    return vec3(1, t * nlen, 0);
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
    const vtkm::Id cylCount = leafs.Get(currentNode);
    for (vtkm::Id i = 1; i <= cylCount; ++i)
    {
      const vtkm::Id cylIndex = leafs.Get(currentNode + i);
      if (cylIndex < CylIds.GetNumberOfValues())
      {
        vtkm::Id3 pointIndex = CylIds.Get(cylIndex);
        vtkm::Float32 radius = Radii.Get(cylIndex);
        vtkm::Vec<Precision, 3> bottom, top;
        bottom = vtkm::Vec<Precision, 3>(points.Get(pointIndex[1]));
        top = vtkm::Vec<Precision, 3>(points.Get(pointIndex[2]));

        vtkm::Vec<vtkm::Float32, 3> ret;
        ret = cylinder(origin, dir, bottom, top, radius);
        if (ret[0] > 0)
        {
          if (ret[1] < closestDistance && ret[1] > minDistance)
          {
            //matid = vtkm::Vec<, 3>(points.Get(cur_offset + 2))[0];
            closestDistance = ret[1];
            hitIndex = cylIndex;
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
                            CylinderIntersector* self,
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

    vtkm::Id3 cylId = indicesPortal.Get(hitIndex);

    vtkm::Vec<Precision, 3> a, b;
    a = points.Get(cylId[1]);
    b = points.Get(cylId[2]);

    vtkm::Vec<Precision, 3> ap, ab;
    ap = intersection - a;
    ab = b - a;

    Precision mag2 = vtkm::Magnitude(ab);
    Precision len = vtkm::dot(ab, ap);
    Precision t = len / mag2;

    vtkm::Vec<Precision, 3> center;
    center = a + t * ab;

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

    //TODO: this should be interpolated?
    vtkm::Id3 pointId = indicesPortal.Get(hitIndex);

    scalar = Precision(scalars.Get(pointId[0]));
    //normalize
    scalar = (scalar - MinScalar) * invDeltaScalar;
  }
}; //class GetScalar

} // namespace detail

CylinderIntersector::CylinderIntersector()
  : ShapeIntersector()
{
}

CylinderIntersector::~CylinderIntersector()
{
}


void CylinderIntersector::IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex)
{
  vtkm::cont::TryExecute(detail::IntersectFunctor(), this, rays, returnCellIndex);
}

void CylinderIntersector::IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex)
{
  vtkm::cont::TryExecute(detail::IntersectFunctor(), this, rays, returnCellIndex);
}

template <typename Device, typename Precision>
void CylinderIntersector::IntersectRaysImp(Device,
                                           Ray<Precision>& rays,
                                           bool vtkmNotUsed(returnCellIndex))
{

  detail::CylinderLeafIntersector<Device> leafIntersector(this->CylIds, Radii);

  BVHTraverser<detail::CylinderLeafIntersector> traverser;
  traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle, Device());

  RayOperations::UpdateRayStatus(rays);
}

template <typename Precision>
void CylinderIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                              const vtkm::cont::Field* scalarField,
                                              const vtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);

  // TODO: if this is nodes of a mesh, support points
  bool isSupportedField =
    (scalarField->GetAssociation() == vtkm::cont::Field::Association::POINTS ||
     scalarField->GetAssociation() == vtkm::cont::Field::Association::CELL_SET);
  if (!isSupportedField)
    throw vtkm::cont::ErrorBadValue("Field not accociated with a cell set");

  vtkm::worklet::DispatcherMapField<detail::CalculateNormals>(detail::CalculateNormals())
    .Invoke(rays.HitIdx,
            rays.Intersection,
            rays.NormalX,
            rays.NormalY,
            rays.NormalZ,
            CoordsHandle,
            CylIds);

  vtkm::worklet::DispatcherMapField<detail::GetScalar<Precision>>(
    detail::GetScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
    .Invoke(rays.HitIdx, rays.Scalar, *scalarField, CylIds);
}

void CylinderIntersector::IntersectionData(Ray<vtkm::Float32>& rays,
                                           const vtkm::cont::Field* scalarField,
                                           const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

void CylinderIntersector::IntersectionData(Ray<vtkm::Float64>& rays,
                                           const vtkm::cont::Field* scalarField,
                                           const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

vtkm::Id CylinderIntersector::GetNumberOfShapes() const
{
  return CylIds.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
