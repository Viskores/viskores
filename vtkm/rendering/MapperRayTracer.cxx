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

#include <vtkm/cont/BoundsCompute.h>
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

struct MapperRayTracer::CompareIndices
{
  vtkm::Vec3f CameraDirection;
  vtkm::Vec3f* Centers;
  CompareIndices(vtkm::Vec3f* centers, vtkm::Vec3f cameraDirection)
    : CameraDirection(cameraDirection)
    , Centers(centers)
  {
  }

  bool operator()(int i, int j) const
  {
    return (vtkm::Dot(Centers[i], CameraDirection) < vtkm::Dot(Centers[j], CameraDirection));
  }
};

void MapperRayTracer::RenderCellsPartitioned(const vtkm::cont::PartitionedDataSet partitionedData,
                                             const std::string fieldName,
                                             const vtkm::cont::ColorTable& colorTable,
                                             const vtkm::rendering::Camera& camera,
                                             const vtkm::Range& scalarRange)
{
  // sort partitions back to front for best rendering with the volume renderer
  vtkm::Vec3f centers[partitionedData.GetNumberOfPartitions()];
  std::vector<int> indices(partitionedData.GetNumberOfPartitions());
  for (unsigned int p = 0; p < partitionedData.GetNumberOfPartitions(); p++)
  {
    indices[p] = p;
    centers[p] = vtkm::cont::BoundsCompute(partitionedData.GetPartition(p)).Center();
  }
  CompareIndices comparator(centers, camera.GetLookAt() - camera.GetPosition());
  std::sort(indices.begin(), indices.end(), comparator);

  for (unsigned int p = 0; p < partitionedData.GetNumberOfPartitions(); p++)
  {
    auto partition = partitionedData.GetPartition(indices[p]);
    this->RenderCellsImpl(partition.GetCellSet(),
                          partition.GetCoordinateSystem(),
                          partition.GetField(fieldName.c_str()),
                          colorTable,
                          camera,
                          scalarRange,
                          partition.GetGhostCellField());
  }
}

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

MapperRayTracer::~MapperRayTracer() {}

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

void MapperRayTracer::RenderCellsImpl(const vtkm::cont::UnknownCellSet& cellset,
                                      const vtkm::cont::CoordinateSystem& coords,
                                      const vtkm::cont::Field& scalarField,
                                      const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                      const vtkm::rendering::Camera& camera,
                                      const vtkm::Range& scalarRange,
                                      const vtkm::cont::Field& ghostField)
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
  triExtractor.ExtractCells(cellset, ghostField);

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
  vtkm::Int32 width = (vtkm::Int32)this->Internals->Canvas->GetWidth();
  vtkm::Int32 height = (vtkm::Int32)this->Internals->Canvas->GetHeight();

  this->Internals->RayCamera.SetParameters(camera, width, height);

  this->Internals->RayCamera.CreateRays(this->Internals->Rays, shapeBounds);
  this->Internals->Tracer.GetCamera() = this->Internals->RayCamera;
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

vtkm::rendering::Mapper* MapperRayTracer::NewCopy() const
{
  return new vtkm::rendering::MapperRayTracer(*this);
}
}
} // vtkm::rendering
