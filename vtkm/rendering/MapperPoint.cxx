//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperPoint.h>

#include <vtkm/cont/Timer.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/internal/RunTriangulator.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/SphereExtractor.h>
#include <vtkm/rendering/raytracing/SphereIntersector.h>

namespace vtkm
{
namespace rendering
{

struct MapperPoint::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;
  bool CompositeBackground;
  vtkm::Float32 PointRadius;
  bool UseNodes;
  vtkm::Float32 PointDelta;
  bool UseVariableRadius;

  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , PointRadius(-1.f)
    , UseNodes(true)
    , PointDelta(0.5f)
    , UseVariableRadius(false)
  {
  }
};

MapperPoint::MapperPoint()
  : Internals(new InternalsType)
{
}

MapperPoint::~MapperPoint()
{
}

void MapperPoint::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Internals->Canvas = dynamic_cast<CanvasRayTracer*>(canvas);
    if (this->Internals->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue("MapperPoint: bad canvas type. Must be CanvasRayTracer");
    }
  }
  else
  {
    this->Internals->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas* MapperPoint::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperPoint::UseCells()
{
  this->Internals->UseNodes = false;
}
void MapperPoint::UseNodes()
{
  this->Internals->UseNodes = true;
}

void MapperPoint::SetRadius(const vtkm::Float32& radius)
{
  if (radius <= 0.f)
  {
    throw vtkm::cont::ErrorBadValue("MapperPoint: point radius must be positive");
  }
  this->Internals->PointRadius = radius;
}

void MapperPoint::SetRadiusDelta(const vtkm::Float32& delta)
{
  this->Internals->PointDelta = delta;
}

void MapperPoint::UseVariableRadius(bool useVariableRadius)
{
  this->Internals->UseVariableRadius = useVariableRadius;
}

void MapperPoint::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                              const vtkm::cont::CoordinateSystem& coords,
                              const vtkm::cont::Field& scalarField,
                              const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                              const vtkm::rendering::Camera& camera,
                              const vtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  // make sure we start fresh
  this->Internals->Tracer.Clear();

  logger->OpenLogEntry("mapper_ray_tracer");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;

  vtkm::Bounds coordBounds = coords.GetBounds();
  vtkm::Float32 baseRadius = this->Internals->PointRadius;
  if (baseRadius == -1.f)
  {
    // set a default radius
    vtkm::Float64 lx = coordBounds.X.Length();
    vtkm::Float64 ly = coordBounds.Y.Length();
    vtkm::Float64 lz = coordBounds.Z.Length();
    vtkm::Float64 mag = vtkm::Sqrt(lx * lx + ly * ly + lz * lz);
    // same as used in vtk ospray
    constexpr vtkm::Float64 heuristic = 500.;
    baseRadius = static_cast<vtkm::Float32>(mag / heuristic);
  }

  vtkm::Bounds shapeBounds;

  raytracing::SphereExtractor sphereExtractor;

  if (this->Internals->UseVariableRadius)
  {
    vtkm::Float32 minRadius = baseRadius - baseRadius * this->Internals->PointDelta;
    vtkm::Float32 maxRadius = baseRadius + baseRadius * this->Internals->PointDelta;
    if (this->Internals->UseNodes)
    {

      sphereExtractor.ExtractCoordinates(coords, scalarField, minRadius, maxRadius);
    }
    else
    {
      sphereExtractor.ExtractCells(cellset, scalarField, minRadius, maxRadius);
    }
  }
  else
  {
    if (this->Internals->UseNodes)
    {

      sphereExtractor.ExtractCoordinates(coords, baseRadius);
    }
    else
    {
      sphereExtractor.ExtractCells(cellset, baseRadius);
    }
  }

  if (sphereExtractor.GetNumberOfSpheres() > 0)
  {
    auto sphereIntersector = std::make_shared<raytracing::SphereIntersector>();
    sphereIntersector->SetData(coords, sphereExtractor.GetPointIds(), sphereExtractor.GetRadii());
    this->Internals->Tracer.AddShapeIntersector(sphereIntersector);
    shapeBounds.Include(sphereIntersector->GetShapeBounds());
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

void MapperPoint::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperPoint::StartScene()
{
  // Nothing needs to be done.
}

void MapperPoint::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperPoint::NewCopy() const
{
  return new vtkm::rendering::MapperPoint(*this);
}
}
} // vtkm::rendering
