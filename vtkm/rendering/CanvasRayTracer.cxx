//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/CanvasRayTracer.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{

namespace internal
{

class ClearBuffers : public vtkm::worklet::WorkletMapField
{
  vtkm::rendering::Color ClearColor;

public:
  VTKM_CONT
  ClearBuffers(const vtkm::rendering::Color& clearColor)
    : ClearColor(clearColor)
  {
  }
  typedef void ControlSignature(FieldOut<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);
  VTKM_EXEC
  void operator()(vtkm::Vec<vtkm::Float32, 4>& color, vtkm::Float32& depth) const
  {
    color = this->ClearColor.Components;
    depth = 1.001f;
  }
}; //class ClearBuffers

struct ClearBuffersInvokeFunctor
{
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;
  typedef vtkm::rendering::Canvas::DepthBufferType DepthBufferType;

  ClearBuffers Worklet;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;

  VTKM_CONT
  ClearBuffersInvokeFunctor(const vtkm::rendering::Color& backgroundColor,
                            const ColorBufferType& colorBuffer,
                            const DepthBufferType& depthBuffer)
    : Worklet(backgroundColor)
    , ColorBuffer(colorBuffer)
    , DepthBuffer(depthBuffer)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::worklet::DispatcherMapField<ClearBuffers, Device> dispatcher(this->Worklet);
    dispatcher.Invoke(this->ColorBuffer, this->DepthBuffer);
    return true;
  }
};

class SurfaceConverter : public vtkm::worklet::WorkletMapField
{
  vtkm::Matrix<vtkm::Float32, 4, 4> ViewProjMat;

public:
  VTKM_CONT
  SurfaceConverter(const vtkm::Matrix<vtkm::Float32, 4, 4> viewProjMat)
    : ViewProjMat(viewProjMat)
  {
  }

  typedef void ControlSignature(FieldIn<>,
                                WholeArrayIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldIn<>,
                                ExecObject,
                                ExecObject);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5, _6, _7, WorkIndex);
  template <typename Precision, typename ColorPortalType>
  VTKM_EXEC void operator()(
    const vtkm::Id& pixelIndex,
    ColorPortalType& colorBufferIn,
    const Precision& inDepth,
    const vtkm::Vec<Precision, 3>& origin,
    const vtkm::Vec<Precision, 3>& dir,
    vtkm::exec::ExecutionWholeArray<vtkm::Float32>& depthBuffer,
    vtkm::exec::ExecutionWholeArray<vtkm::Vec<vtkm::Float32, 4>>& colorBuffer,
    const vtkm::Id& index) const
  {
    vtkm::Vec<Precision, 3> intersection = origin + inDepth * dir;
    vtkm::Vec<vtkm::Float32, 4> point;

    point[0] = static_cast<vtkm::Float32>(intersection[0]);
    point[1] = static_cast<vtkm::Float32>(intersection[1]);
    point[2] = static_cast<vtkm::Float32>(intersection[2]);
    point[3] = 1.f;

    vtkm::Vec<vtkm::Float32, 4> newpoint;
    newpoint = vtkm::MatrixMultiply(this->ViewProjMat, point);
    newpoint[0] = newpoint[0] / newpoint[3];
    newpoint[1] = newpoint[1] / newpoint[3];
    newpoint[2] = newpoint[2] / newpoint[3];

    vtkm::Float32 depth = newpoint[2];

    depth = 0.5f * (-depth) + 0.5f;
    vtkm::Vec<vtkm::Float32, 4> color;
    color[0] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 0));
    color[1] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 1));
    color[2] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 2));
    color[3] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 3));

    vtkm::Float32 existingDepth = depthBuffer.Get(pixelIndex);

    depthBuffer.Set(pixelIndex, depth);
    colorBuffer.Set(pixelIndex, color);
  }
}; //class SurfaceConverter

template <typename Precision>
struct WriteFunctor
{
protected:
  vtkm::rendering::CanvasRayTracer* Canvas;
  const vtkm::rendering::raytracing::Ray<Precision>& Rays;
  const vtkm::cont::ArrayHandle<Precision>& Colors;
  const vtkm::rendering::Camera& CameraView;
  vtkm::Matrix<vtkm::Float32, 4, 4> ViewProjMat;

public:
  VTKM_CONT
  WriteFunctor(vtkm::rendering::CanvasRayTracer* canvas,
               const vtkm::rendering::raytracing::Ray<Precision>& rays,
               const vtkm::cont::ArrayHandle<Precision>& colors,
               const vtkm::rendering::Camera& camera)
    : Canvas(canvas)
    , Rays(rays)
    , Colors(colors)
    , CameraView(camera)
  {
    ViewProjMat = vtkm::MatrixMultiply(
      CameraView.CreateProjectionMatrix(Canvas->GetWidth(), Canvas->GetHeight()),
      CameraView.CreateViewMatrix());
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::worklet::DispatcherMapField<SurfaceConverter, Device>(SurfaceConverter(ViewProjMat))
      .Invoke(
        Rays.PixelIdx,
        Colors,
        Rays.Distance,
        Rays.Origin,
        Rays.Dir,
        vtkm::exec::ExecutionWholeArray<vtkm::Float32>(Canvas->GetDepthBuffer()),
        vtkm::exec::ExecutionWholeArray<vtkm::Vec<vtkm::Float32, 4>>(Canvas->GetColorBuffer()));
    return true;
  }
};

template <typename Precision>
VTKM_CONT void WriteToCanvas(const vtkm::rendering::raytracing::Ray<Precision>& rays,
                             const vtkm::cont::ArrayHandle<Precision>& colors,
                             const vtkm::rendering::Camera& camera,
                             vtkm::rendering::CanvasRayTracer* canvas)
{
  camera.Print();
  WriteFunctor<Precision> functor(canvas, rays, colors, camera);

  vtkm::cont::TryExecute(functor);

  //Force the transfer so the vectors contain data from device
  canvas->GetColorBuffer().GetPortalControl().Get(0);
  canvas->GetDepthBuffer().GetPortalControl().Get(0);
}

} // namespace internal

CanvasRayTracer::CanvasRayTracer(vtkm::Id width, vtkm::Id height)
  : Canvas(width, height)
{
}

CanvasRayTracer::~CanvasRayTracer()
{
}

void CanvasRayTracer::Initialize()
{
  // Nothing to initialize
}

void CanvasRayTracer::Activate()
{
  // Nothing to activate
}

void CanvasRayTracer::Finish()
{
  // Nothing to finish
}

void CanvasRayTracer::Clear()
{
  // TODO: Should the rendering library support policies or some other way to
  // configure with custom devices?
  vtkm::cont::TryExecute(internal::ClearBuffersInvokeFunctor(
    this->GetBackgroundColor(), this->GetColorBuffer(), this->GetDepthBuffer()));
}

void CanvasRayTracer::WriteToCanvas(const vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays,
                                    const vtkm::cont::ArrayHandle<vtkm::Float32>& colors,
                                    const vtkm::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

void CanvasRayTracer::WriteToCanvas(const vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays,
                                    const vtkm::cont::ArrayHandle<vtkm::Float64>& colors,
                                    const vtkm::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

vtkm::rendering::Canvas* CanvasRayTracer::NewCopy() const
{
  return new vtkm::rendering::CanvasRayTracer(*this);
}

void CanvasRayTracer::AddLine(const vtkm::Vec<vtkm::Float64, 2>&,
                              const vtkm::Vec<vtkm::Float64, 2>&,
                              vtkm::Float32,
                              const vtkm::rendering::Color&) const
{
  // Not implemented
}

void CanvasRayTracer::AddColorBar(const vtkm::Bounds&,
                                  const vtkm::rendering::ColorTable&,
                                  bool) const
{
  // Not implemented
}

void CanvasRayTracer::AddText(const vtkm::Vec<vtkm::Float32, 2>&,
                              vtkm::Float32,
                              vtkm::Float32,
                              vtkm::Float32,
                              const vtkm::Vec<vtkm::Float32, 2>&,
                              const vtkm::rendering::Color&,
                              const std::string&) const
{
  // Not implemented
}
}
}
