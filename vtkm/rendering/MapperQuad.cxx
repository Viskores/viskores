//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperQuad.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/Cylinderizer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/QuadExtractor.h>
#include <vtkm/rendering/raytracing/QuadIntersector.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>

namespace vtkm
{
namespace rendering
{

struct MapperQuad::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;
  bool CompositeBackground;
  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
  {
  }
};

MapperQuad::MapperQuad()
  : Internals(new InternalsType)
{
}

MapperQuad::~MapperQuad()
{
}

void MapperQuad::SetCanvas(vtkm::rendering::Canvas* canvas)
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

vtkm::rendering::Canvas* MapperQuad::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperQuad::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
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


  //
  // Add supported shapes
  //
  vtkm::Bounds shapeBounds;
  raytracing::QuadExtractor quadExtractor;
  quadExtractor.ExtractCells(cellset);
  if (quadExtractor.GetNumberOfQuads() > 0)
  {
    auto quadIntersector = std::make_shared<raytracing::QuadIntersector>();
    quadIntersector->SetData(coords, quadExtractor.GetQuadIds());
    this->Internals->Tracer.AddShapeIntersector(quadIntersector);
    shapeBounds.Include(quadIntersector->GetShapeBounds());
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

void MapperQuad::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperQuad::StartScene()
{
  // Nothing needs to be done.
}

void MapperQuad::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperQuad::NewCopy() const
{
  return new vtkm::rendering::MapperQuad(*this);
}
}
} // vtkm::rendering
