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

#include <vtkm/rendering/raytracing/Camera.h>
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


} // namespace detail

ScalarRenderer::ScalarRenderer()
  : NumberOfShapes(0)
{
}

ScalarRenderer::~ScalarRenderer()
{
  Clear();
}

Camera& ScalarRenderer::GetCamera()
{
  return camera;
}


void ScalarRenderer::AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector)
{
  NumberOfShapes += intersector->GetNumberOfShapes();
  Intersectors.push_back(intersector);
}

void ScalarRenderer::AddField(const vtkm::cont::Field& scalarField)
{
  Fields.push_back(scalar_field);
}

void ScalarRenderer::Render(Ray<vtkm::Float32>& rays)
{
  RenderOnDevice(rays);
}

void ScalarRenderer::Render(Ray<vtkm::Float64>& rays)
{
  RenderOnDevice(rays);
}

vtkm::Id ScalarRenderer::GetNumberOfShapes() const
{
  return NumberOfShapes;
}

void ScalarRenderer::Clear()
{
  Intersectors.clear();
}

template <typename Precision>
void ScalarRenderer::RenderOnDevice(Ray<Precision>& rays)
{
  using Timer = vtkm::cont::Timer;

  Logger* logger = Logger::GetInstance();
  Timer renderTimer;
  renderTimer.Start();
  vtkm::Float64 time = 0.;
  logger->OpenLogEntry("ray_tracer");
  logger->AddLogData("device", GetDeviceString());

  logger->AddLogData("shapes", NumberOfShapes);
  logger->AddLogData("num_rays", rays.NumRays);
  size_t numShapes = Intersectors.size();
  if (NumberOfShapes > 0)
  {
    Timer timer;
    timer.Start();

    for (size_t i = 0; i < numShapes; ++i)
    {
      Intersectors[i]->IntersectRays(rays);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersect", time);

      timer.Start();
      Intersectors[i]->IntersectionData(rays, ScalarField, ScalarRange);
      time = timer.GetElapsedTime();
      logger->AddLogData("intersection_data", time);
      timer.Start();

      // Calculate the color at the intersection  point
      //detail::SurfaceColor surfaceColor;
      //surfaceColor.run(rays, ColorMap, camera, this->Shade);

      time = timer.GetElapsedTime();
      logger->AddLogData("shade", time);
    }
  }

  time = renderTimer.GetElapsedTime();
  logger->CloseLogEntry(time);
} // RenderOnDevice
}
}
} // namespace vtkm::rendering::raytracing
