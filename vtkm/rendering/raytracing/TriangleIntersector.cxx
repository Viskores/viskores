//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/raytracing/TriangleIntersector.h>

#include <cstring>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/rendering/raytracing/BVHTraverser.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/TriangleIntersections.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

template <typename Device>
class WaterTightLeafIntersector
{
public:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Id4>;
  using Id4ArrayPortal = typename Id4Handle::ExecutionTypes<Device>::PortalConst;
  Id4ArrayPortal Triangles;

public:
  WaterTightLeafIntersector() = default;

  WaterTightLeafIntersector(const Id4Handle& triangles)
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
class MollerTriLeafIntersector
{
  //protected:
public:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Id4>;
  using Id4ArrayPortal = typename Id4Handle::ExecutionTypes<Device>::PortalConst;
  Id4ArrayPortal Triangles;

public:
  MollerTriLeafIntersector() {}

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

class MollerExecWrapper : public vtkm::cont::ExecutionObjectBase
{
protected:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Id4>;
  Id4Handle Triangles;

public:
  MollerExecWrapper(Id4Handle& triangles)
    : Triangles(triangles)
  {
  }

  template <typename Device>
  VTKM_CONT MollerTriLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return MollerTriLeafIntersector<Device>(Triangles);
  }
};

class WaterTightExecWrapper : public vtkm::cont::ExecutionObjectBase
{
protected:
  using Id4Handle = vtkm::cont::ArrayHandle<vtkm::Id4>;
  Id4Handle Triangles;

public:
  WaterTightExecWrapper(Id4Handle& triangles)
    : Triangles(triangles)
  {
  }

  template <typename Device>
  VTKM_CONT WaterTightLeafIntersector<Device> PrepareForExecution(Device) const
  {
    return WaterTightLeafIntersector<Device>(Triangles);
  }
};

class CellIndexFilter : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CellIndexFilter() {}
  typedef void ControlSignature(FieldInOut, WholeArrayIn);
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
    typedef void ControlSignature(FieldIn,
                                  FieldIn,
                                  FieldIn,
                                  FieldInOut,
                                  WholeArrayIn,
                                  WholeArrayIn);
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

      vtkm::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      //Todo: one normalization
      scalar = Precision(scalars.Get(indices[0]));

      //normalize
      scalar = (scalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar

  template <typename Precision>
  VTKM_CONT void Run(Ray<Precision>& rays,
                     vtkm::cont::ArrayHandle<vtkm::Id4> triangles,
                     vtkm::cont::CoordinateSystem coordsHandle,
                     const vtkm::cont::Field scalarField,
                     const vtkm::Range& scalarRange)
  {
    const bool isSupportedField = scalarField.IsFieldCell() || scalarField.IsFieldPoint();
    if (!isSupportedField)
    {
      throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
    }
    const bool isAssocPoints = scalarField.IsFieldPoint();

    // Find the triangle normal
    vtkm::worklet::DispatcherMapField<CalculateNormals>(CalculateNormals())
      .Invoke(
        rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, coordsHandle, triangles);

    // Calculate scalar value at intersection point
    if (isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField<LerpScalar<Precision>>(
        LerpScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx,
                rays.U,
                rays.V,
                rays.Scalar,
                scalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                triangles);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<NodalScalar<Precision>>(
        NodalScalar<Precision>(vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx,
                rays.Scalar,
                scalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                triangles);
    }
  } // Run

}; // Class IntersectionData

#define AABB_EPSILON 0.00001f
class FindTriangleAABBs : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  FindTriangleAABBs() {}
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
  VTKM_EXEC void operator()(const vtkm::Id4 indices,
                            vtkm::Float32& xmin,
                            vtkm::Float32& ymin,
                            vtkm::Float32& zmin,
                            vtkm::Float32& xmax,
                            vtkm::Float32& ymax,
                            vtkm::Float32& zmax,
                            const PointPortalType& points) const
  {
    // cast to Float32
    vtkm::Vec3f_32 point;
    point = static_cast<vtkm::Vec3f_32>(points.Get(indices[1]));
    xmin = point[0];
    ymin = point[1];
    zmin = point[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    point = static_cast<vtkm::Vec3f_32>(points.Get(indices[2]));
    xmin = vtkm::Min(xmin, point[0]);
    ymin = vtkm::Min(ymin, point[1]);
    zmin = vtkm::Min(zmin, point[2]);
    xmax = vtkm::Max(xmax, point[0]);
    ymax = vtkm::Max(ymax, point[1]);
    zmax = vtkm::Max(zmax, point[2]);
    point = static_cast<vtkm::Vec3f_32>(points.Get(indices[3]));
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

TriangleIntersector::TriangleIntersector()
  : UseWaterTight(false)
{
}

void TriangleIntersector::SetUseWaterTight(bool useIt)
{
  UseWaterTight = useIt;
}

void TriangleIntersector::SetData(const vtkm::cont::CoordinateSystem& coords,
                                  vtkm::cont::ArrayHandle<vtkm::Id4> triangles)
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

vtkm::cont::ArrayHandle<vtkm::Id4> TriangleIntersector::GetTriangles()
{
  return Triangles;
}



VTKM_CONT void TriangleIntersector::IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}


VTKM_CONT void TriangleIntersector::IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
VTKM_CONT void TriangleIntersector::IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex)
{
  if (UseWaterTight)
  {
    detail::WaterTightExecWrapper leafIntersector(this->Triangles);
    BVHTraverser traverser;
    traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);
  }
  else
  {
    detail::MollerExecWrapper leafIntersector(this->Triangles);

    BVHTraverser traverser;
    traverser.IntersectRays(rays, this->BVH, leafIntersector, this->CoordsHandle);
  }
  // Normally we return the index of the triangle hit,
  // but in some cases we are only interested in the cell
  if (returnCellIndex)
  {
    vtkm::worklet::DispatcherMapField<detail::CellIndexFilter> cellIndexFilterDispatcher;
    cellIndexFilterDispatcher.Invoke(rays.HitIdx, Triangles);
  }
  // Update ray status
  RayOperations::UpdateRayStatus(rays);
}

VTKM_CONT void TriangleIntersector::IntersectionData(Ray<vtkm::Float32>& rays,
                                                     const vtkm::cont::Field scalarField,
                                                     const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

VTKM_CONT void TriangleIntersector::IntersectionData(Ray<vtkm::Float64>& rays,
                                                     const vtkm::cont::Field scalarField,
                                                     const vtkm::Range& scalarRange)
{
  IntersectionDataImp(rays, scalarField, scalarRange);
}

template <typename Precision>
VTKM_CONT void TriangleIntersector::IntersectionDataImp(Ray<Precision>& rays,
                                                        const vtkm::cont::Field scalarField,
                                                        const vtkm::Range& scalarRange)
{
  ShapeIntersector::IntersectionPoint(rays);
  detail::TriangleIntersectionData intData;
  intData.Run(rays, this->Triangles, this->CoordsHandle, scalarField, scalarRange);
}

vtkm::Id TriangleIntersector::GetNumberOfShapes() const
{
  return Triangles.GetNumberOfValues();
}
}
}
} //namespace vtkm::rendering::raytracing
