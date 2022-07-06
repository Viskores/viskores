//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/raytracing/ScalarRenderer.h>

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
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

template <typename Precision>
class SurfaceShade
{
public:
  class Shade : public vtkm::worklet::WorkletMapField
  {
  private:
    vtkm::Vec3f_32 LightPosition;
    vtkm::Vec3f_32 LightAmbient;
    vtkm::Vec3f_32 LightDiffuse;
    vtkm::Vec3f_32 LightSpecular;
    vtkm::Float32 SpecularExponent;
    vtkm::Vec3f_32 CameraPosition;
    vtkm::Vec3f_32 LookAt;
    Precision MissScalar;

  public:
    VTKM_CONT
    Shade(const vtkm::Vec3f_32& lightPosition,
          const vtkm::Vec3f_32& cameraPosition,
          const vtkm::Vec3f_32& lookAt,
          const Precision missScalar)
      : LightPosition(lightPosition)
      , CameraPosition(cameraPosition)
      , LookAt(lookAt)
      , MissScalar(missScalar)
    {
      //Set up some default lighting parameters for now
      LightAmbient[0] = .5f;
      LightAmbient[1] = .5f;
      LightAmbient[2] = .5f;
      LightDiffuse[0] = .7f;
      LightDiffuse[1] = .7f;
      LightDiffuse[2] = .7f;
      LightSpecular[0] = .7f;
      LightSpecular[1] = .7f;
      LightSpecular[2] = .7f;
      SpecularExponent = 20.f;
    }

    using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldOut);
    using ExecutionSignature = void(_1, _2, _3, _4);
    //using ExecutionSignature = void(_1, _2, _3, _4, WorkIndex);

    //template <typename ShadePortalType, typename Precision>
    VTKM_EXEC void operator()(const vtkm::Id& hitIdx,
                              const vtkm::Vec<Precision, 3>& normal,
                              const vtkm::Vec<Precision, 3>& intersection,
                              Precision& output) const //,
    //                              const vtkm::Id& idx) const
    {

      if (hitIdx < 0)
      {
        output = MissScalar;
      }

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
      Precision specularConstant =
        vtkm::Pow(vtkm::Max(cosPhi, zero), static_cast<Precision>(SpecularExponent));

      Precision shade = vtkm::Min(
        LightAmbient[0] + LightDiffuse[0] * cosTheta + LightSpecular[0] * specularConstant, one);
      output = shade;
    }

    vtkm::Vec3f_32 GetDiffuse() { return LightDiffuse; }

    vtkm::Vec3f_32 GetAmbient() { return LightAmbient; }

    vtkm::Vec3f_32 GetSpecular() { return LightSpecular; }

    vtkm::Float32 GetSpecularExponent() { return SpecularExponent; }


    void SetDiffuse(vtkm::Vec3f_32 newDiffuse)
    {
      LightDiffuse[0] = newDiffuse[0];
      LightDiffuse[1] = newDiffuse[1];
      LightDiffuse[2] = newDiffuse[2];
    }

    void SetAmbient(vtkm::Vec3f_32 newAmbient)
    {
      LightAmbient[0] = newAmbient[0];
      LightAmbient[1] = newAmbient[1];
      LightAmbient[2] = newAmbient[2];
    }

    void SetSpecular(vtkm::Vec3f_32 newSpecular)
    {
      LightSpecular[0] = newSpecular[0];
      LightSpecular[1] = newSpecular[1];
      LightSpecular[2] = newSpecular[2];
    }

    void SetSpecularExponent(vtkm::Float32 newExponent) { SpecularExponent = newExponent; }


  }; //class Shade

  //template <typename Precision>
  VTKM_CONT void run(Ray<Precision>& rays,
                     const vtkm::rendering::raytracing::Camera& camera,
                     const Precision missScalar,
                     vtkm::cont::ArrayHandle<Precision> shadings,
                     bool shade)
  {
    if (shade)
    {
      // TODO: support light positions
      vtkm::Vec3f_32 scale(2, 2, 2);
      vtkm::Vec3f_32 lightPosition = camera.GetPosition() + scale * camera.GetUp();
      vtkm::worklet::DispatcherMapField<Shade>(
        Shade(lightPosition, camera.GetPosition(), camera.GetLookAt(), missScalar))
        .Invoke(rays.HitIdx, rays.Normal, rays.Intersection, shadings);
    }
  }
}; // class SurfaceShade


template <typename Precision>
class FilterDepth : public vtkm::worklet::WorkletMapField
{
private:
  Precision MissScalar;

public:
  VTKM_CONT
  FilterDepth(const Precision missScalar)
    : MissScalar(missScalar)
  {
  }

  typedef void ControlSignature(FieldIn, FieldInOut);

  typedef void ExecutionSignature(_1, _2);
  VTKM_EXEC void operator()(const vtkm::Id& hitIndex, Precision& scalar) const
  {
    Precision value = scalar;
    if (hitIndex < 0)
    {
      value = MissScalar;
    }

    scalar = value;
  }
}; //class FilterDepth

template <typename Precision>
class WriteBuffer : public vtkm::worklet::WorkletMapField
{
private:
  Precision MissScalar;

public:
  VTKM_CONT
  WriteBuffer(const Precision missScalar)
    : MissScalar(missScalar)
  {
  }

  typedef void ControlSignature(FieldIn, FieldIn, FieldOut);

  typedef void ExecutionSignature(_1, _2, _3);
  VTKM_EXEC void operator()(const vtkm::Id& hitIndex,
                            const Precision& scalar,
                            Precision& output) const
  {
    Precision value = scalar;
    if (hitIndex < 0)
    {
      value = MissScalar;
    }

    output = value;
  }
}; //class WriteBuffer

template <typename Precision>
class WriteDepthBuffer : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  WriteDepthBuffer() {}

  typedef void ControlSignature(FieldIn, FieldOut);

  typedef void ExecutionSignature(_1, _2);
  VTKM_EXEC void operator()(const Precision& depth, Precision& output) const { output = depth; }
}; //class WriteDepthBuffer
} // namespace detail

ScalarRenderer::ScalarRenderer()
  : IntersectorValid(false)
{
}

ScalarRenderer::~ScalarRenderer() {}

void ScalarRenderer::SetShapeIntersector(std::shared_ptr<ShapeIntersector> intersector)
{
  Intersector = intersector;
  IntersectorValid = true;
}

void ScalarRenderer::AddField(const vtkm::cont::Field& scalarField)
{
  vtkm::cont::ArrayHandle<vtkm::Range> ranges = scalarField.GetRange();
  if (ranges.GetNumberOfValues() != 1)
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer(AddField): field must be a scalar");
  }
  Fields.push_back(scalarField);
}

void ScalarRenderer::Render(Ray<vtkm::Float32>& rays,
                            vtkm::Float32 missScalar,
                            vtkm::rendering::raytracing::Camera& cam)
{
  RenderOnDevice(rays, missScalar, cam);
}

void ScalarRenderer::Render(Ray<vtkm::Float64>& rays,
                            vtkm::Float64 missScalar,
                            vtkm::rendering::raytracing::Camera& cam)
{
  RenderOnDevice(rays, missScalar, cam);
}

template <typename Precision>
void ScalarRenderer::RenderOnDevice(Ray<Precision>& rays,
                                    Precision missScalar,
                                    vtkm::rendering::raytracing::Camera& cam)
{
  using Timer = vtkm::cont::Timer;

  Logger* logger = Logger::GetInstance();
  Timer renderTimer;
  renderTimer.Start();
  vtkm::Float64 time = 0.;
  logger->OpenLogEntry("scalar_renderer");
  logger->AddLogData("device", GetDeviceString());

  logger->AddLogData("num_rays", rays.NumRays);
  const size_t numFields = Fields.size();

  if (numFields == 0)
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer: no fields added");
  }
  if (!IntersectorValid)
  {
    throw vtkm::cont::ErrorBadValue("ScalarRenderer: intersector never set");
  }

  Timer timer;
  timer.Start();

  Intersector->IntersectRays(rays);
  time = timer.GetElapsedTime();
  logger->AddLogData("intersect", time);

  for (size_t f = 0; f < numFields; ++f)
  {
    timer.Start();
    Intersector->IntersectionData(rays, Fields[f]);
    time = timer.GetElapsedTime();
    logger->AddLogData("intersection_data", time);
    AddBuffer(rays, missScalar, Fields[f].GetName());
  }



  const vtkm::Int32 numChannels = 1;
  ChannelBuffer<Precision> buffer(numChannels, rays.NumRays);
  detail::SurfaceShade<Precision> surfaceShade;
  surfaceShade.run(rays, cam, missScalar, buffer.Buffer, true);
  buffer.SetName("shading");
  rays.Buffers.push_back(buffer);


  vtkm::worklet::DispatcherMapField<detail::FilterDepth<Precision>>(
    detail::FilterDepth<Precision>(missScalar))
    .Invoke(rays.HitIdx, rays.Distance);

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice

template <typename Precision>
void ScalarRenderer::AddBuffer(Ray<Precision>& rays, Precision missScalar, const std::string name)
{
  const vtkm::Int32 numChannels = 1;
  ChannelBuffer<Precision> buffer(numChannels, rays.NumRays);

  vtkm::worklet::DispatcherMapField<detail::WriteBuffer<Precision>>(
    detail::WriteBuffer<Precision>(missScalar))
    .Invoke(rays.HitIdx, rays.Scalar, buffer.Buffer);

  buffer.SetName(name);
  rays.Buffers.push_back(buffer);
}

template <typename Precision>
void ScalarRenderer::AddDepthBuffer(Ray<Precision>& rays)
{
  const vtkm::Int32 numChannels = 1;
  ChannelBuffer<Precision> buffer(numChannels, rays.NumRays);

  vtkm::worklet::DispatcherMapField<detail::WriteDepthBuffer<Precision>>(
    detail::WriteDepthBuffer<Precision>())
    .Invoke(rays.Depth, buffer.Buffer);

  buffer.SetName("depth");
  rays.Buffers.push_back(buffer);
}
}
}
} // namespace vtkm::rendering::raytracing
