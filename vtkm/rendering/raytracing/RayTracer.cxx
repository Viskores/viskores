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

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{

class SurfaceColor
{
public:
  class Shade : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Vec<vtkm::Float32, 3> LightPosition;
    vtkm::Vec<vtkm::Float32, 3> LightAbmient;
    vtkm::Vec<vtkm::Float32, 3> LightDiffuse;
    vtkm::Vec<vtkm::Float32, 3> LightSpecular;
    vtkm::Float32 SpecularExponent;
    vtkm::Vec<vtkm::Float32, 3> CameraPosition;
    vtkm::Vec<vtkm::Float32, 3> LookAt;

  public:
    VTKM_CONT
    Shade(const vtkm::Vec<vtkm::Float32, 3>& lightPosition,
          const vtkm::Vec<vtkm::Float32, 3>& cameraPosition,
          const vtkm::Vec<vtkm::Float32, 3>& lookAt)
      : LightPosition(lightPosition)
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

    using ControlSignature =
      void(FieldIn<>, FieldIn<>, FieldIn<>, FieldIn<>, WholeArrayInOut<>, WholeArrayIn<>);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

    template <typename ColorPortalType, typename Precision, typename ColorMapPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& hitIdx,
                              const Precision& scalar,
                              const vtkm::Vec<Precision, 3>& normal,
                              const vtkm::Vec<Precision, 3>& intersection,
                              ColorPortalType& colors,
                              ColorMapPortalType colorMap,
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
      Precision cosTheta = vtkm::dot(normal, lightDir);
      //clamp tp [0,1]
      const Precision zero = 0.f;
      const Precision one = 1.f;
      cosTheta = vtkm::Min(vtkm::Max(cosTheta, zero), one);
      //Specular lighting
      vtkm::Vec<Precision, 3> reflect = 2.f * vtkm::dot(lightDir, normal) * normal - lightDir;
      vtkm::Normalize(reflect);
      Precision cosPhi = vtkm::dot(reflect, viewDir);
      Precision specularConstant = Precision(pow(vtkm::Max(cosPhi, zero), SpecularExponent));
      vtkm::Int32 colorMapSize = static_cast<vtkm::Int32>(colorMap.GetNumberOfValues());
      vtkm::Int32 colorIdx = vtkm::Int32(scalar * Precision(colorMapSize - 1));

      // clamp color index
      colorIdx = vtkm::Max(0, colorIdx);
      colorIdx = vtkm::Min(colorMapSize - 1, colorIdx);
      color = colorMap.Get(colorIdx);

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

  }; //class Shade

  class MapScalarToColor : public vtkm::worklet::WorkletMapField
  {
  public:
    VTKM_CONT
    MapScalarToColor() {}

    using ControlSignature = void(FieldIn<>, FieldIn<>, WholeArrayInOut<>, WholeArrayIn<>);
    using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

    template <typename ColorPortalType, typename Precision, typename ColorMapPortalType>
    VTKM_EXEC void operator()(const vtkm::Id& hitIdx,
                              const Precision& scalar,
                              ColorPortalType& colors,
                              ColorMapPortalType colorMap,
                              const vtkm::Id& idx) const
    {

      if (hitIdx < 0)
      {
        return;
      }

      vtkm::Vec<Precision, 4> color;
      vtkm::Id offset = idx * 4;

      vtkm::Int32 colorMapSize = static_cast<vtkm::Int32>(colorMap.GetNumberOfValues());
      vtkm::Int32 colorIdx = vtkm::Int32(scalar * Precision(colorMapSize - 1));

      // clamp color index
      colorIdx = vtkm::Max(0, colorIdx);
      colorIdx = vtkm::Min(colorMapSize - 1, colorIdx);
      color = colorMap.Get(colorIdx);

      colors.Set(offset + 0, color[0]);
      colors.Set(offset + 1, color[1]);
      colors.Set(offset + 2, color[2]);
      colors.Set(offset + 3, color[3]);
    }

  }; //class MapScalarToColor

  template <typename Precision>
  VTKM_CONT void run(Ray<Precision>& rays,
                     vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap,
                     const vtkm::rendering::raytracing::Camera& camera,
                     bool shade)
  {
    if (shade)
    {
      // TODO: support light positions
      vtkm::Vec<vtkm::Float32, 3> scale(2, 2, 2);
      vtkm::Vec<vtkm::Float32, 3> lightPosition = camera.GetPosition() + scale * camera.GetUp();
      vtkm::worklet::DispatcherMapField<Shade>(
        Shade(lightPosition, camera.GetPosition(), camera.GetLookAt()))
        .Invoke(rays.HitIdx,
                rays.Scalar,
                rays.Normal,
                rays.Intersection,
                rays.Buffers.at(0).Buffer,
                colorMap);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<MapScalarToColor>(MapScalarToColor())
        .Invoke(rays.HitIdx, rays.Scalar, rays.Buffers.at(0).Buffer, colorMap);
    }
  }
}; // class SurfaceColor

} // namespace detail

RayTracer::RayTracer()
  : NumberOfShapes(0)
  , Shade(true)
{
}

RayTracer::~RayTracer()
{
  Clear();
}

Camera& RayTracer::GetCamera()
{
  return camera;
}


void RayTracer::AddShapeIntersector(ShapeIntersector* intersector)
{
  NumberOfShapes += intersector->GetNumberOfShapes();
  Intersectors.push_back(intersector);
}

void RayTracer::SetField(const vtkm::cont::Field& scalarField, const vtkm::Range& scalarRange)
{
  ScalarField = &scalarField;
  ScalarRange = scalarRange;
}

void RayTracer::SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap)
{
  ColorMap = colorMap;
}

void RayTracer::Render(Ray<vtkm::Float32>& rays)
{
  RenderOnDevice(rays);
}

void RayTracer::Render(Ray<vtkm::Float64>& rays)
{
  RenderOnDevice(rays);
}

void RayTracer::SetShadingOn(bool on)
{
  Shade = on;
}

vtkm::Id RayTracer::GetNumberOfShapes() const
{
  return NumberOfShapes;
}

void RayTracer::Clear()
{
  size_t numShapes = Intersectors.size();
  for (size_t i = 0; i < numShapes; ++i)
  {
    delete Intersectors[i];
  }

  Intersectors.clear();
}

template <typename Precision>
void RayTracer::RenderOnDevice(Ray<Precision>& rays)
{
  using Timer = vtkm::cont::Timer<vtkm::cont::DeviceAdapterTagSerial>;

  Logger* logger = Logger::GetInstance();
  Timer renderTimer;
  vtkm::Float64 time = 0.;
  logger->OpenLogEntry("ray_tracer");
  logger->AddLogData("device", GetDeviceString());

  logger->AddLogData("shapes", NumberOfShapes);
  logger->AddLogData("num_rays", rays.NumRays);
  size_t numShapes = Intersectors.size();
  if (NumberOfShapes > 0)
  {
    Timer timer;

    for (size_t i = 0; i < numShapes; ++i)
    {
      Intersectors[i]->IntersectRays(rays);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersect", time);

      timer.Reset();
      Intersectors[i]->IntersectionData(rays, ScalarField, ScalarRange);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersection_data", time);
      timer.Reset();

      // Calculate the color at the intersection  point
      detail::SurfaceColor surfaceColor;
      surfaceColor.run(rays, ColorMap, camera, this->Shade);

      time = timer.GetElapsedTime();
      logger->AddLogData("shade", time);
      timer.Reset();
    }
  }

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice
}
}
} // namespace vtkm::rendering::raytracing
