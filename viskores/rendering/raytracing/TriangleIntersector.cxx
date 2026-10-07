//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#include <viskores/rendering/raytracing/Texture.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>

#include <cstring>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/rendering/raytracing/BVHTraverser.h>
#include <viskores/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/TriangleIntersections.h>

#include <viskores/worklet/WorkletMapField.h>

namespace viskores
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
  using Id4Handle = viskores::cont::ArrayHandle<viskores::Id4>;
  using Id4ArrayPortal = typename Id4Handle::ReadPortalType;
  Id4ArrayPortal Triangles;

public:
  WaterTightLeafIntersector() = default;

  WaterTightLeafIntersector(const Id4Handle& triangles, viskores::cont::Token& token)
    : Triangles(triangles.PrepareForInput(Device(), token))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VISKORES_EXEC inline void IntersectLeaf(const viskores::Int32& currentNode,
                                          const viskores::Vec<Precision, 3>& origin,
                                          const viskores::Vec<Precision, 3>& dir,
                                          const PointPortalType& points,
                                          viskores::Id& hitIndex,
                                          Precision& closestDistance,
                                          Precision& minU,
                                          Precision& minV,
                                          LeafPortalType leafs,
                                          const Precision& minDistance) const
  {
    const viskores::Id triangleCount = leafs.Get(currentNode);
    WaterTight intersector;
    for (viskores::Id i = 1; i <= triangleCount; ++i)
    {
      const viskores::Id triIndex = leafs.Get(currentNode + i);
      viskores::Vec<Id, 4> triangle = Triangles.Get(triIndex);
      viskores::Vec<Precision, 3> a = viskores::Vec<Precision, 3>(points.Get(triangle[1]));
      viskores::Vec<Precision, 3> b = viskores::Vec<Precision, 3>(points.Get(triangle[2]));
      viskores::Vec<Precision, 3> c = viskores::Vec<Precision, 3>(points.Get(triangle[3]));
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
  using Id4Handle = viskores::cont::ArrayHandle<viskores::Id4>;
  using Id4ArrayPortal = typename Id4Handle::ReadPortalType;
  Id4ArrayPortal Triangles;

public:
  MollerTriLeafIntersector() {}

  MollerTriLeafIntersector(const Id4Handle& triangles, viskores::cont::Token& token)
    : Triangles(triangles.PrepareForInput(Device(), token))
  {
  }

  template <typename PointPortalType, typename LeafPortalType, typename Precision>
  VISKORES_EXEC inline void IntersectLeaf(const viskores::Int32& currentNode,
                                          const viskores::Vec<Precision, 3>& origin,
                                          const viskores::Vec<Precision, 3>& dir,
                                          const PointPortalType& points,
                                          viskores::Id& hitIndex,
                                          Precision& closestDistance,
                                          Precision& minU,
                                          Precision& minV,
                                          LeafPortalType leafs,
                                          const Precision& minDistance) const
  {
    const viskores::Id triangleCount = leafs.Get(currentNode);
    Moller intersector;
    for (viskores::Id i = 1; i <= triangleCount; ++i)
    {
      const viskores::Id triIndex = leafs.Get(currentNode + i);
      viskores::Vec<Id, 4> triangle = Triangles.Get(triIndex);
      viskores::Vec<Precision, 3> a = viskores::Vec<Precision, 3>(points.Get(triangle[1]));
      viskores::Vec<Precision, 3> b = viskores::Vec<Precision, 3>(points.Get(triangle[2]));
      viskores::Vec<Precision, 3> c = viskores::Vec<Precision, 3>(points.Get(triangle[3]));
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

class MollerExecWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using Id4Handle = viskores::cont::ArrayHandle<viskores::Id4>;
  Id4Handle Triangles;

public:
  MollerExecWrapper(Id4Handle& triangles)
    : Triangles(triangles)
  {
  }

  template <typename Device>
  VISKORES_CONT MollerTriLeafIntersector<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    return MollerTriLeafIntersector<Device>(this->Triangles, token);
  }
};

class WaterTightExecWrapper : public viskores::cont::ExecutionObjectBase
{
protected:
  using Id4Handle = viskores::cont::ArrayHandle<viskores::Id4>;
  Id4Handle Triangles;

public:
  WaterTightExecWrapper(Id4Handle& triangles)
    : Triangles(triangles)
  {
  }

  template <typename Device>
  VISKORES_CONT WaterTightLeafIntersector<Device> PrepareForExecution(
    Device,
    viskores::cont::Token& token) const
  {
    return WaterTightLeafIntersector<Device>(this->Triangles, token);
  }
};

class CellIndexFilter : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
  CellIndexFilter() {}
  typedef void ControlSignature(FieldInOut, WholeArrayIn);
  typedef void ExecutionSignature(_1, _2);
  template <typename TrianglePortalType>
  VISKORES_EXEC void operator()(viskores::Id& hitIndex, TrianglePortalType& triangles) const
  {
    viskores::Id cellIndex = -1;
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
  // Worklet to calutate the normals of a triangle if
  // none are stored in the data set
  class CalculateNormals : public viskores::worklet::WorkletMapField
  {
  public:
    VISKORES_CONT
    CalculateNormals() {}
    typedef void
      ControlSignature(FieldIn, FieldIn, FieldOut, FieldOut, FieldOut, WholeArrayIn, WholeArrayIn);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
    template <typename Precision, typename PointPortalType, typename IndicesPortalType>
    VISKORES_EXEC inline void operator()(const viskores::Id& hitIndex,
                                         const viskores::Vec<Precision, 3>& rayDir,
                                         Precision& normalX,
                                         Precision& normalY,
                                         Precision& normalZ,
                                         const PointPortalType& points,
                                         const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      viskores::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);
      viskores::Vec<Precision, 3> a = points.Get(indices[1]);
      viskores::Vec<Precision, 3> b = points.Get(indices[2]);
      viskores::Vec<Precision, 3> c = points.Get(indices[3]);

      viskores::Vec<Precision, 3> normal = viskores::TriangleNormal(a, b, c);
      viskores::Normalize(normal);

      //flip the normal if its pointing the wrong way
      if (viskores::dot(normal, rayDir) > 0.f)
        normal = -normal;
      normalX = normal[0];
      normalY = normal[1];
      normalZ = normal[2];
    }
  }; //class CalculateNormals

  template <typename Precision>
  class LerpScalar : public viskores::worklet::WorkletMapField
  {
  private:
    TextureCoordinateTransform<Precision> Transform;

  public:
    VISKORES_CONT
    explicit LerpScalar(const viskores::cont::ArrayHandle<viskores::Range>& textureRanges)
      : Transform(textureRanges)
    {
    }
    typedef void ControlSignature(FieldIn,
                                  FieldIn,
                                  FieldIn,
                                  FieldInOut,
                                  FieldInOut,
                                  WholeArrayIn,
                                  WholeArrayIn);
    typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7);
    template <typename TexturePortalType, typename IndicesPortalType>
    VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                  const Precision& u,
                                  const Precision& v,
                                  Precision& textureR,
                                  Precision& textureS,
                                  const TexturePortalType& texture,
                                  const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      viskores::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      Precision n = 1.f - u - v;
      const auto a = MakeTextureCoordinates<Precision>(texture.Get(indices[1]));
      const auto b = MakeTextureCoordinates<Precision>(texture.Get(indices[2]));
      const auto c = MakeTextureCoordinates<Precision>(texture.Get(indices[3]));
      const auto coordinates = this->Transform(a * n + b * u + c * v);
      textureR = coordinates[0];
      textureS = coordinates[1];
    }
  }; //class LerpScalar

  template <typename Precision>
  class NodalScalar : public viskores::worklet::WorkletMapField
  {
  private:
    TextureCoordinateTransform<Precision> Transform;

  public:
    VISKORES_CONT
    explicit NodalScalar(const viskores::cont::ArrayHandle<viskores::Range>& textureRanges)
      : Transform(textureRanges)
    {
    }

    typedef void ControlSignature(FieldIn, FieldOut, FieldOut, WholeArrayIn, WholeArrayIn);

    typedef void ExecutionSignature(_1, _2, _3, _4, _5);
    template <typename TexturePortalType, typename IndicesPortalType>
    VISKORES_EXEC void operator()(const viskores::Id& hitIndex,
                                  Precision& textureR,
                                  Precision& textureS,
                                  const TexturePortalType& texture,
                                  const IndicesPortalType& indicesPortal) const
    {
      if (hitIndex < 0)
        return;

      viskores::Vec<Id, 4> indices = indicesPortal.Get(hitIndex);

      const auto coordinates =
        this->Transform(MakeTextureCoordinates<Precision>(texture.Get(indices[0])));
      textureR = coordinates[0];
      textureS = coordinates[1];
    }
  }; //class LerpScalar

  template <typename Precision>
  VISKORES_CONT void Run(Ray<Precision>& rays,
                         viskores::cont::ArrayHandle<viskores::Id4> triangles,
                         viskores::cont::CoordinateSystem coordsHandle,
                         const viskores::cont::Field textureField,
                         const viskores::cont::ArrayHandle<viskores::Range>& textureRanges)
  {
    const bool isSupportedField = textureField.IsCellField() || textureField.IsPointField();
    if (!isSupportedField)
    {
      throw viskores::cont::ErrorBadValue("Field not associated with cell set or points");
    }
    const bool isAssocPoints = textureField.IsPointField();

    // Find the triangle normal
    viskores::worklet::DispatcherMapField<CalculateNormals>(CalculateNormals())
      .Invoke(
        rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, coordsHandle, triangles);

    // Calculate scalar value at intersection point
    if (isAssocPoints)
    {
      viskores::worklet::DispatcherMapField<LerpScalar<Precision>>(
        LerpScalar<Precision>(textureRanges))
        .Invoke(rays.HitIdx,
                rays.U,
                rays.V,
                rays.TextureR,
                rays.TextureS,
                viskores::rendering::raytracing::GetTextureFieldArray(textureField),
                triangles);
    }
    else
    {
      viskores::worklet::DispatcherMapField<NodalScalar<Precision>>(
        NodalScalar<Precision>(textureRanges))
        .Invoke(rays.HitIdx,
                rays.TextureR,
                rays.TextureS,
                viskores::rendering::raytracing::GetTextureFieldArray(textureField),
                triangles);
    }
  } // Run

}; // Class IntersectionData

#define AABB_EPSILON 0.00001f
class FindTriangleAABBs : public viskores::worklet::WorkletMapField
{
public:
  VISKORES_CONT
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
  VISKORES_EXEC void operator()(const viskores::Id4 indices,
                                viskores::Float32& xmin,
                                viskores::Float32& ymin,
                                viskores::Float32& zmin,
                                viskores::Float32& xmax,
                                viskores::Float32& ymax,
                                viskores::Float32& zmax,
                                const PointPortalType& points) const
  {
    // cast to Float32
    viskores::Vec3f_32 point;
    point = static_cast<viskores::Vec3f_32>(points.Get(indices[1]));
    xmin = point[0];
    ymin = point[1];
    zmin = point[2];
    xmax = xmin;
    ymax = ymin;
    zmax = zmin;
    point = static_cast<viskores::Vec3f_32>(points.Get(indices[2]));
    xmin = viskores::Min(xmin, point[0]);
    ymin = viskores::Min(ymin, point[1]);
    zmin = viskores::Min(zmin, point[2]);
    xmax = viskores::Max(xmax, point[0]);
    ymax = viskores::Max(ymax, point[1]);
    zmax = viskores::Max(zmax, point[2]);
    point = static_cast<viskores::Vec3f_32>(points.Get(indices[3]));
    xmin = viskores::Min(xmin, point[0]);
    ymin = viskores::Min(ymin, point[1]);
    zmin = viskores::Min(zmin, point[2]);
    xmax = viskores::Max(xmax, point[0]);
    ymax = viskores::Max(ymax, point[1]);
    zmax = viskores::Max(zmax, point[2]);


    viskores::Float32 xEpsilon, yEpsilon, zEpsilon;
    const viskores::Float32 minEpsilon = 1e-6f;
    xEpsilon = viskores::Max(minEpsilon, AABB_EPSILON * (xmax - xmin));
    yEpsilon = viskores::Max(minEpsilon, AABB_EPSILON * (ymax - ymin));
    zEpsilon = viskores::Max(minEpsilon, AABB_EPSILON * (zmax - zmin));

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

void TriangleIntersector::SetData(const viskores::cont::CoordinateSystem& coords,
                                  viskores::cont::ArrayHandle<viskores::Id4> triangles)
{

  CoordsHandle = coords;
  Triangles = triangles;

  viskores::rendering::raytracing::AABBs AABB;
  viskores::worklet::DispatcherMapField<detail::FindTriangleAABBs>(detail::FindTriangleAABBs())
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

viskores::cont::ArrayHandle<viskores::Id4> TriangleIntersector::GetTriangles()
{
  return Triangles;
}



VISKORES_CONT void TriangleIntersector::IntersectRays(Ray<viskores::Float32>& rays,
                                                      bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}


VISKORES_CONT void TriangleIntersector::IntersectRays(Ray<viskores::Float64>& rays,
                                                      bool returnCellIndex)
{
  IntersectRaysImp(rays, returnCellIndex);
}

template <typename Precision>
VISKORES_CONT void TriangleIntersector::IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex)
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
    viskores::worklet::DispatcherMapField<detail::CellIndexFilter> cellIndexFilterDispatcher;
    cellIndexFilterDispatcher.Invoke(rays.HitIdx, Triangles);
  }
  // Update ray status
  RayOperations::UpdateRayStatus(rays);
}

VISKORES_CONT void TriangleIntersector::IntersectionData(
  Ray<viskores::Float32>& rays,
  const viskores::cont::Field textureField,
  const viskores::cont::ArrayHandle<viskores::Range>& textureRanges)
{
  IntersectionDataImp(rays, textureField, textureRanges);
}

VISKORES_CONT void TriangleIntersector::IntersectionData(
  Ray<viskores::Float64>& rays,
  const viskores::cont::Field textureField,
  const viskores::cont::ArrayHandle<viskores::Range>& textureRanges)
{
  IntersectionDataImp(rays, textureField, textureRanges);
}

template <typename Precision>
VISKORES_CONT void TriangleIntersector::IntersectionDataImp(
  Ray<Precision>& rays,
  const viskores::cont::Field textureField,
  const viskores::cont::ArrayHandle<viskores::Range>& textureRanges)
{
  ShapeIntersector::IntersectionPoint(rays);
  detail::TriangleIntersectionData intData;
  intData.Run(rays, this->Triangles, this->CoordsHandle, textureField, textureRanges);
}

viskores::Id TriangleIntersector::GetNumberOfShapes() const
{
  return Triangles.GetNumberOfValues();
}
}
}
} //namespace viskores::rendering::raytracing
