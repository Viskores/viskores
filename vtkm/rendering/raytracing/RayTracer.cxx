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
#include <vtkm/rendering/raytracing/RayTracer.h>

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class IntersectionPoint : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  IntersectionPoint() {}
  using ControlSignature =
    void(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>, FieldOut<>, FieldOut<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  template <typename Precision>
  VTKM_EXEC inline void operator()(const vtkm::Id& hitIndex,
                                   const Precision& distance,
                                   const vtkm::Vec<Precision, 3>& rayDir,
                                   const vtkm::Vec<Precision, 3>& rayOrigin,
                                   Precision& intersectionX,
                                   Precision& intersectionY,
                                   Precision& intersectionZ) const
  {
    if (hitIndex < 0)
      return;

    intersectionX = rayOrigin[0] + rayDir[0] * distance;
    intersectionY = rayOrigin[1] + rayDir[1] * distance;
    intersectionZ = rayOrigin[2] + rayDir[2] * distance;
  }
}; //class IntersectionPoint

template <typename Device>
class IntersectionData
{
public:
  // Worklet to calutate the normals of a triagle if
  // none are stored in the data set
  class CalculateNormals : public vtkm::worklet::WorkletMapField
  {
  private:
    using Vec4IntArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>;
    using IndicesArrayPortal = typename Vec4IntArrayHandle::ExecutionTypes<Device>::PortalConst;

    IndicesArrayPortal IndicesPortal;

  public:
    VTKM_CONT
    CalculateNormals(const Vec4IntArrayHandle& indices)
      : IndicesPortal(indices.PrepareForInput(Device()))
    {
    }
    using ControlSignature = void(FieldIn<>,
                                  FieldIn<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  FieldOut<>,
                                  WholeArrayIn<Vec3RenderingTypes>);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
    template <typename Precision, typename PointPortalType>
    VTKM_EXEC inline void operator()(const vtkm::Id& hitIndex,
                                     const vtkm::Vec<Precision, 3>& rayDir,
                                     Precision& normalX,
                                     Precision& normalY,
                                     Precision& normalZ,
                                     const PointPortalType& points) const
    {
      if (hitIndex < 0)
        return;

      vtkm::Vec<Int32, 4> indices = IndicesPortal.Get(hitIndex);
      vtkm::Vec<Precision, 3> a = points.Get(indices[1]);
      vtkm::Vec<Precision, 3> b = points.Get(indices[2]);
      vtkm::Vec<Precision, 3> c = points.Get(indices[3]);

      vtkm::Vec<Precision, 3> normal = vtkm::TriangleNormal(a, b, c);
      vtkm::Normalize(normal);

      //flip the normal if its pointing the wrong way
      if (vtkm::Dot(normal, rayDir) > 0.f)
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
    using Vec4IntArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>;
    using IndicesArrayPortal = typename Vec4IntArrayHandle::ExecutionTypes<Device>::PortalConst;

    IndicesArrayPortal IndicesPortal;
    Precision MinScalar;
    Precision invDeltaScalar;

  public:
    VTKM_CONT
    LerpScalar(const Vec4IntArrayHandle& indices,
               const vtkm::Float32& minScalar,
               const vtkm::Float32& maxScalar)
      : IndicesPortal(indices.PrepareForInput(Device()))
      , MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      if (maxScalar - MinScalar != 0.f)
        invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else
        invDeltaScalar = 1.f / minScalar;
    }
    using ControlSignature =
      void(FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>, WholeArrayIn<ScalarRenderingTypes>);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);
    template <typename ScalarPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                              const Precision& u,
                              const Precision& v,
                              Precision& lerpedScalar,
                              const ScalarPortalType& scalars) const
    {
      if (hitIndex < 0)
        return;

      vtkm::Vec<Int32, 4> indices = IndicesPortal.Get(hitIndex);

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
    using Vec4IntArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Int32, 4>>;
    using IndicesArrayPortal = typename Vec4IntArrayHandle::ExecutionTypes<Device>::PortalConst;

    IndicesArrayPortal IndicesPortal;
    Precision MinScalar;
    Precision invDeltaScalar;

  public:
    VTKM_CONT
    NodalScalar(const Vec4IntArrayHandle& indices,
                const vtkm::Float32& minScalar,
                const vtkm::Float32& maxScalar)
      : IndicesPortal(indices.PrepareForInput(Device()))
      , MinScalar(minScalar)
    {
      //Make sure the we don't divide by zero on
      //something like an iso-surface
      if (maxScalar - MinScalar != 0.f)
        invDeltaScalar = 1.f / (maxScalar - MinScalar);
      else
        invDeltaScalar = 1.f / minScalar;
    }

    using ControlSignature = void(FieldIn<>, FieldOut<>, WholeArrayIn<ScalarRenderingTypes>);

    using ExecutionSignature = void(_1, _2, _3);
    template <typename ScalarPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                              Precision& scalar,
                              const ScalarPortalType& scalars) const
    {
      if (hitIndex < 0)
        return;

      vtkm::Vec<Int32, 4> indices = IndicesPortal.Get(hitIndex);

      //Todo: one normalization
      scalar = Precision(scalars.Get(indices[0]));

      //normalize
      scalar = (scalar - MinScalar) * invDeltaScalar;
    }
  }; //class LerpScalar
  template <typename Precision>
  VTKM_CONT void run(Ray<Precision>& rays,
                     LinearBVH& bvh,
                     vtkm::cont::ArrayHandleVirtualCoordinates& coordsHandle,
                     const vtkm::cont::Field& scalarField,
                     const vtkm::Range& scalarRange)
  {
    bool isSupportedField =
      (scalarField.GetAssociation() == vtkm::cont::Field::Association::POINTS ||
       scalarField.GetAssociation() == vtkm::cont::Field::Association::CELL_SET);
    if (!isSupportedField)
      throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
    bool isAssocPoints = scalarField.GetAssociation() == vtkm::cont::Field::Association::POINTS;

    vtkm::worklet::DispatcherMapField<CalculateNormals, Device>(CalculateNormals(bvh.LeafNodes))
      .Invoke(rays.HitIdx, rays.Dir, rays.NormalX, rays.NormalY, rays.NormalZ, coordsHandle);

    if (isAssocPoints)
    {
      vtkm::worklet::DispatcherMapField<LerpScalar<Precision>, Device>(
        LerpScalar<Precision>(
          bvh.LeafNodes, vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx, rays.U, rays.V, rays.Scalar, scalarField);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<NodalScalar<Precision>, Device>(
        NodalScalar<Precision>(
          bvh.LeafNodes, vtkm::Float32(scalarRange.Min), vtkm::Float32(scalarRange.Max)))
        .Invoke(rays.HitIdx, rays.Scalar, scalarField);
    }
  } // Run

}; // Class IntersectionData

template <typename Device>
class SurfaceColor
{
public:
  class MapScalarToColor : public vtkm::worklet::WorkletMapField
  {
  private:
    using ColorArrayHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
    using ColorArrayPortal = typename ColorArrayHandle::ExecutionTypes<Device>::PortalConst;

    ColorArrayPortal ColorMap;
    vtkm::Int32 ColorMapSize;
    vtkm::Vec<vtkm::Float32, 3> LightPosition;
    vtkm::Vec<vtkm::Float32, 3> LightAbmient;
    vtkm::Vec<vtkm::Float32, 3> LightDiffuse;
    vtkm::Vec<vtkm::Float32, 3> LightSpecular;
    vtkm::Float32 SpecularExponent;
    vtkm::Vec<vtkm::Float32, 3> CameraPosition;
    vtkm::Vec<vtkm::Float32, 3> LookAt;

  public:
    VTKM_CONT
    MapScalarToColor(const ColorArrayHandle& colorMap,
                     const vtkm::Int32& colorMapSize,
                     const vtkm::Vec<vtkm::Float32, 3>& lightPosition,
                     const vtkm::Vec<vtkm::Float32, 3>& cameraPosition,
                     const vtkm::Vec<vtkm::Float32, 3>& lookAt)
      : ColorMap(colorMap.PrepareForInput(Device()))
      , ColorMapSize(colorMapSize)
      , LightPosition(lightPosition)
      , CameraPosition(cameraPosition)
      , LookAt(lookAt)
    {
      //Set up some default lighting parameters for now
      LightAbmient[0] = .5f;
      LightAbmient[1] = .5f;
      LightAbmient[2] = .5f;
      LightDiffuse[0] = .7f;
      LightDiffuse[1] = .7f;
      LightDiffuse[2] = .7f;
      LightSpecular[0] = .7f;
      LightSpecular[1] = .7f;
      LightSpecular[2] = .7f;
      SpecularExponent = 20.f;
    }
    using ControlSignature = void(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, WholeArrayInOut<>);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex);
    template <typename ColorPortalType, typename Precision>
    VTKM_EXEC void operator()(const vtkm::Id& hitIdx,
                              const Precision& scalar,
                              const vtkm::Vec<Precision, 3>& normal,
                              const vtkm::Vec<Precision, 3>& intersection,
                              ColorPortalType& colors,
                              const vtkm::Id& idx) const
    {
      vtkm::Vec<Precision, 4> color;
      vtkm::Id offset = idx * 4;

      if (hitIdx < 0)
      {
        return;
      }

      color[0] = colors.Get(offset + 0);
      color[1] = colors.Get(offset + 1);
      color[2] = colors.Get(offset + 2);
      color[3] = colors.Get(offset + 3);

      vtkm::Vec<Precision, 3> lightDir = LightPosition - intersection;
      vtkm::Vec<Precision, 3> viewDir = CameraPosition - LookAt;
      vtkm::Normalize(lightDir);
      vtkm::Normalize(viewDir);
      //Diffuse lighting
      Precision cosTheta = vtkm::Dot(normal, lightDir);
      //clamp tp [0,1]
      const Precision zero = 0.f;
      const Precision one = 1.f;
      cosTheta = vtkm::Min(vtkm::Max(cosTheta, zero), one);
      //Specular lighting
      vtkm::Vec<Precision, 3> reflect = 2.f * vtkm::Dot(lightDir, normal) * normal - lightDir;
      vtkm::Normalize(reflect);
      Precision cosPhi = vtkm::Dot(reflect, viewDir);
      Precision specularConstant =
        Precision(pow(vtkm::Max(cosPhi, zero), (Precision)SpecularExponent));
      vtkm::Int32 colorIdx = vtkm::Int32(scalar * Precision(ColorMapSize - 1));

      //Just in case clamp the value to the valid range
      colorIdx = (colorIdx < 0) ? 0 : colorIdx;
      colorIdx = (colorIdx > ColorMapSize - 1) ? ColorMapSize - 1 : colorIdx;
      color = ColorMap.Get(colorIdx);

      color[0] *= vtkm::Min(
        LightAbmient[0] + LightDiffuse[0] * cosTheta + LightSpecular[0] * specularConstant, one);
      color[1] *= vtkm::Min(
        LightAbmient[1] + LightDiffuse[1] * cosTheta + LightSpecular[1] * specularConstant, one);
      color[2] *= vtkm::Min(
        LightAbmient[2] + LightDiffuse[2] * cosTheta + LightSpecular[2] * specularConstant, one);

      colors.Set(offset + 0, color[0]);
      colors.Set(offset + 1, color[1]);
      colors.Set(offset + 2, color[2]);
      colors.Set(offset + 3, color[3]);
    }

  }; //class MapScalarToColor

  template <typename Precision>
  VTKM_CONT void run(Ray<Precision>& rays,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap,
                     const vtkm::rendering::raytracing::Camera& camera)
  {
    // TODO: support light positions
    vtkm::Vec<vtkm::Float32, 3> scale(2, 2, 2);
    vtkm::Vec<vtkm::Float32, 3> lightPosition = camera.GetPosition() + scale * camera.GetUp();
    const vtkm::Int32 colorMapSize = vtkm::Int32(colorMap.GetNumberOfValues());
    vtkm::worklet::DispatcherMapField<MapScalarToColor, Device>(
      MapScalarToColor(
        colorMap, colorMapSize, lightPosition, camera.GetPosition(), camera.GetLookAt()))
      .Invoke(rays.HitIdx, rays.Scalar, rays.Normal, rays.Intersection, rays.Buffers.at(0).Buffer);
  }
}; // class SurfaceColor

} // namespace detail

RayTracer::RayTracer()
{
}

Camera& RayTracer::GetCamera()
{
  return camera;
}

void RayTracer::SetData(const vtkm::cont::ArrayHandleVirtualCoordinates& coordsHandle,
                        const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& indices,
                        vtkm::cont::Field& scalarField,
                        const vtkm::Id& numberOfTriangles,
                        const vtkm::Range& scalarRange,
                        const vtkm::Bounds& dataBounds)
{
  CoordsHandle = coordsHandle;
  Indices = indices;
  ScalarField = scalarField;
  NumberOfTriangles = numberOfTriangles;
  ScalarRange = scalarRange;
  DataBounds = dataBounds;
  Bvh.SetData(coordsHandle, indices, DataBounds);
}


void RayTracer::SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap)
{
  ColorMap = colorMap;
}

template <typename Precision>
struct RayTracer::RenderFunctor
{
protected:
  vtkm::rendering::raytracing::RayTracer* Self;
  vtkm::rendering::raytracing::Ray<Precision>& Rays;

public:
  VTKM_CONT
  RenderFunctor(vtkm::rendering::raytracing::RayTracer* self,
                vtkm::rendering::raytracing::Ray<Precision>& rays)
    : Self(self)
    , Rays(rays)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    this->Self->RenderOnDevice(this->Rays, Device());
    return true;
  }
};

void RayTracer::Render(Ray<vtkm::Float32>& rays)
{
  RenderFunctor<vtkm::Float32> functor(this, rays);
  vtkm::cont::TryExecute(functor);
}
void

RayTracer::Render(Ray<vtkm::Float64> &rays)
{
  RenderFunctor<vtkm::Float64> functor(this, rays);
  vtkm::cont::TryExecute(functor);
}


template <typename Device, typename Precision>
void RayTracer::RenderOnDevice(Ray<Precision>& rays, Device)
{
  rays.EnableIntersectionData(Device());

  Logger* logger = Logger::GetInstance();
  vtkm::cont::Timer<Device> renderTimer;
  vtkm::Float64 time = 0.;
  logger->OpenLogEntry("ray_tracer");
  logger->AddLogData("device", GetDeviceString(Device()));

  Bvh.ConstructOnDevice(Device());
  logger->AddLogData("triangles", NumberOfTriangles);
  logger->AddLogData("num_rays", rays.NumRays);

  if (NumberOfTriangles > 0)
  {
    vtkm::cont::Timer<Device> timer;
    // Find distance to intersection
    TriangleIntersector<Device, TriLeafIntersector<Moller>> intersector;
    intersector.run(rays, Bvh, CoordsHandle);
    time = timer.GetElapsedTime();
    logger->AddLogData("intersect", time);
    timer.Reset();

    // Calculate normal and scalar value (TODO: find a better name)
    detail::IntersectionData<Device> intData;
    intData.run(rays, Bvh, CoordsHandle, ScalarField, ScalarRange);

    time = timer.GetElapsedTime();
    logger->AddLogData("intersection_data", time);
    timer.Reset();

    // Find the intersection point from hit distance
    vtkm::worklet::DispatcherMapField<detail::IntersectionPoint, Device>(
      detail::IntersectionPoint())
      .Invoke(rays.HitIdx,
              rays.Distance,
              rays.Dir,
              rays.Origin,
              rays.IntersectionX,
              rays.IntersectionY,
              rays.IntersectionZ);

    time = timer.GetElapsedTime();
    logger->AddLogData("find_point", time);
    timer.Reset();

    // Calculate the color at the intersection  point
    detail::SurfaceColor<Device> surfaceColor;
    surfaceColor.run(rays, ColorMap, camera);

    time = timer.GetElapsedTime();
    logger->AddLogData("shade", time);
    timer.Reset();
  }

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice
}
}
} // namespace vtkm::rendering::raytracing
