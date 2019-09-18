//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperVolume.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/VolumeRendererStructured.h>

#include <sstream>

#define DEFAULT_SAMPLE_DISTANCE -1.f

namespace vtkm
{
namespace rendering
{

struct MapperVolume::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::Float32 SampleDistance;
  bool CompositeBackground;

  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , SampleDistance(DEFAULT_SAMPLE_DISTANCE)
    , CompositeBackground(true)
  {
  }
};

MapperVolume::MapperVolume()
  : Internals(new InternalsType)
{
}

MapperVolume::~MapperVolume()
{
}

void MapperVolume::SetCanvas(vtkm::rendering::Canvas* canvas)
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

vtkm::rendering::Canvas* MapperVolume::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperVolume::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                               const vtkm::cont::CoordinateSystem& coords,
                               const vtkm::cont::Field& scalarField,
                               const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                               const vtkm::rendering::Camera& camera,
                               const vtkm::Range& scalarRange)
{
  if (!cellset.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    std::stringstream msg;
    std::string theType = typeid(cellset).name();
    msg << "Mapper volume: cell set type not currently supported\n";
    msg << "Type : " << theType << std::endl;
    throw vtkm::cont::ErrorBadValue(msg.str());
  }
  else
  {
    raytracing::Logger* logger = raytracing::Logger::GetInstance();
    logger->OpenLogEntry("mapper_volume");
    vtkm::cont::Timer tot_timer;
    tot_timer.Start();
    vtkm::cont::Timer timer;

    vtkm::rendering::raytracing::VolumeRendererStructured tracer;

    vtkm::rendering::raytracing::Camera rayCamera;
    vtkm::rendering::raytracing::Ray<vtkm::Float32> rays;

    rayCamera.SetParameters(camera, *this->Internals->Canvas);

    rayCamera.CreateRays(rays, coords.GetBounds());
    rays.Buffers.at(0).InitConst(0.f);
    raytracing::RayOperations::MapCanvasToRays(rays, camera, *this->Internals->Canvas);


    if (this->Internals->SampleDistance != DEFAULT_SAMPLE_DISTANCE)
    {
      tracer.SetSampleDistance(this->Internals->SampleDistance);
    }

    tracer.SetData(
      coords, scalarField, cellset.Cast<vtkm::cont::CellSetStructured<3>>(), scalarRange);
    tracer.SetColorMap(this->ColorMap);

    tracer.Render(rays);

    timer.Start();
    this->Internals->Canvas->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);

    if (this->Internals->CompositeBackground)
    {
      this->Internals->Canvas->BlendBackground();
    }
    vtkm::Float64 time = timer.GetElapsedTime();
    logger->AddLogData("write_to_canvas", time);
    time = tot_timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }
}

void MapperVolume::StartScene()
{
  // Nothing needs to be done.
}

void MapperVolume::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperVolume::NewCopy() const
{
  return new vtkm::rendering::MapperVolume(*this);
}

void MapperVolume::SetSampleDistance(const vtkm::Float32 sampleDistance)
{
  this->Internals->SampleDistance = sampleDistance;
}

void MapperVolume::SetCompositeBackground(const bool compositeBackground)
{
  this->Internals->CompositeBackground = compositeBackground;
}
}
} // namespace vtkm::rendering
