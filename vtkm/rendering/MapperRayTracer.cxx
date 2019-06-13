//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperRayTracer.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/SphereExtractor.h>
#include <vtkm/rendering/raytracing/SphereIntersector.h>
#include <vtkm/rendering/raytracing/TriangleExtractor.h>

namespace vtkm
{
namespace rendering
{

struct MapperRayTracer::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;
  bool CompositeBackground;
  bool Shade;
  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , Shade(true)
  {
  }
};

MapperRayTracer::MapperRayTracer()
  : Internals(new InternalsType)
{
}

MapperRayTracer::~MapperRayTracer()
{
}

void MapperRayTracer::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if (this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue("Ray Tracer: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas* MapperRayTracer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperRayTracer::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                                  const vtkm::cont::CoordinateSystem& coords,
                                  const vtkm::cont::Field& scalarField,
                                  const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                  const vtkm::rendering::Camera& camera,
                                  const vtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_ray_tracer");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;

  // make sure we start fresh
  this->Internals->Tracer.Clear();
  //
  // Add supported shapes
  //
  vtkm::Bounds shapeBounds;
  raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cellset);
  if (triExtractor.GetNumberOfTriangles() > 0)
  {
    auto triIntersector = std::make_shared<raytracing::TriangleIntersector>();
    triIntersector->SetData(coords, triExtractor.GetTriangles());
    this->Internals->Tracer.AddShapeIntersector(triIntersector);
    shapeBounds.Include(triIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  vtkm::rendering::raytracing::Camera& cam = this->Internals->Tracer.GetCamera();
  cam.SetParameters(camera, *this->Internals->Canvas);
  this->Internals->RayCamera.SetParameters(camera, *this->Internals->Canvas);

  this->Internals->RayCamera.CreateRays(this->Internals->Rays, shapeBounds);
  this->Internals->Rays.Buffers.at(0).InitConst(0.f);
  raytracing::RayOperations::MapCanvasToRays(
    this->Internals->Rays, camera, *this->Internals->Canvas);



  this->Internals->Tracer.SetField(scalarField, scalarRange);

  this->Internals->Tracer.SetColorMap(this->ColorMap);
  this->Internals->Tracer.SetShadingOn(this->Internals->Shade);
  this->Internals->Tracer.Render(this->Internals->Rays);

  timer.Start();
  this->Internals->Canvas->WriteToCanvas(
    this->Internals->Rays, this->Internals->Rays.Buffers.at(0).Buffer, camera);

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

void MapperRayTracer::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperRayTracer::SetShadingOn(bool on)
{
  this->Internals->Shade = on;
}

void MapperRayTracer::StartScene()
{
  // Nothing needs to be done.
}

void MapperRayTracer::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperRayTracer::NewCopy() const
{
  return new vtkm::rendering::MapperRayTracer(*this);
}
}
} // vtkm::rendering
