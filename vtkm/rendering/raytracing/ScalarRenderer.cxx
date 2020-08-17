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
}; //class WriteBuffer

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

void ScalarRenderer::Render(Ray<vtkm::Float32>& rays, vtkm::Float32 missScalar)
{
  RenderOnDevice(rays, missScalar);
}

void ScalarRenderer::Render(Ray<vtkm::Float64>& rays, vtkm::Float64 missScalar)
{
  RenderOnDevice(rays, missScalar);
}

template <typename Precision>
void ScalarRenderer::RenderOnDevice(Ray<Precision>& rays, Precision missScalar)
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
