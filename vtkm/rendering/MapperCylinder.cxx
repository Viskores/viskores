//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperCylinder.h>

#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/Cylinderizer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/CylinderExtractor.h>
#include <vtkm/rendering/raytracing/CylinderIntersector.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/Worklets.h>

namespace vtkm
{
namespace rendering
{

class CalcDistance : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CalcDistance(const vtkm::Vec3f_32& _eye_pos)
    : eye_pos(_eye_pos)
  {
  }
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef void ExecutionSignature(_1, _2);
  template <typename VecType, typename OutType>
  VTKM_EXEC inline void operator()(const VecType& pos, OutType& out) const
  {
    VecType tmp = eye_pos - pos;
    out = static_cast<OutType>(vtkm::Sqrt(vtkm::dot(tmp, tmp)));
  }

  const vtkm::Vec3f_32 eye_pos;
}; //class CalcDistance

struct MapperCylinder::InternalsType
{
  vtkm::rendering::CanvasRayTracer* Canvas;
  vtkm::rendering::raytracing::RayTracer Tracer;
  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;
  bool CompositeBackground;
  vtkm::Float32 Radius;
  vtkm::Float32 Delta;
  bool UseVariableRadius;
  VTKM_CONT
  InternalsType()
    : Canvas(nullptr)
    , CompositeBackground(true)
    , Radius(-1.0f)
    , Delta(0.5)
    , UseVariableRadius(false)
  {
  }
};

MapperCylinder::MapperCylinder()
  : Internals(new InternalsType)
{
}

MapperCylinder::~MapperCylinder()
{
}

void MapperCylinder::SetCanvas(vtkm::rendering::Canvas* canvas)
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

vtkm::rendering::Canvas* MapperCylinder::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperCylinder::UseVariableRadius(bool useVariableRadius)
{
  this->Internals->UseVariableRadius = useVariableRadius;
}

void MapperCylinder::SetRadius(const vtkm::Float32& radius)
{
  if (radius <= 0.f)
  {
    throw vtkm::cont::ErrorBadValue("MapperCylinder: radius must be positive");
  }
  this->Internals->Radius = radius;
}
void MapperCylinder::SetRadiusDelta(const vtkm::Float32& delta)
{
  this->Internals->Delta = delta;
}

void MapperCylinder::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                                 const vtkm::cont::CoordinateSystem& coords,
                                 const vtkm::cont::Field& scalarField,
                                 const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                 const vtkm::rendering::Camera& camera,
                                 const vtkm::Range& scalarRange)
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();
  logger->OpenLogEntry("mapper_cylinder");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;


  vtkm::Bounds shapeBounds;
  raytracing::CylinderExtractor cylExtractor;

  vtkm::Float32 baseRadius = this->Internals->Radius;
  if (baseRadius == -1.f)
  {
    // set a default radius
    vtkm::cont::ArrayHandle<vtkm::Float32> dist;
    vtkm::worklet::DispatcherMapField<CalcDistance>(CalcDistance(camera.GetPosition()))
      .Invoke(coords, dist);


    vtkm::Float32 min_dist =
      vtkm::cont::Algorithm::Reduce(dist, vtkm::Infinity<vtkm::Float32>(), vtkm::Minimum());

    baseRadius = 0.576769694f * min_dist - 0.603522029f * vtkm::Pow(vtkm::Float32(min_dist), 2.f) +
      0.232171175f * vtkm::Pow(vtkm::Float32(min_dist), 3.f) -
      0.038697244f * vtkm::Pow(vtkm::Float32(min_dist), 4.f) +
      0.002366979f * vtkm::Pow(vtkm::Float32(min_dist), 5.f);
    baseRadius /= min_dist;
    vtkm::worklet::DispatcherMapField<vtkm::rendering::raytracing::MemSet<vtkm::Float32>>(
      vtkm::rendering::raytracing::MemSet<vtkm::Float32>(baseRadius))
      .Invoke(cylExtractor.GetRadii());
  }

  if (this->Internals->UseVariableRadius)
  {
    vtkm::Float32 minRadius = baseRadius - baseRadius * this->Internals->Delta;
    vtkm::Float32 maxRadius = baseRadius + baseRadius * this->Internals->Delta;

    cylExtractor.ExtractCells(cellset, scalarField, minRadius, maxRadius);
  }
  else
  {
    cylExtractor.ExtractCells(cellset, baseRadius);
  }

  //
  // Add supported shapes
  //

  if (cylExtractor.GetNumberOfCylinders() > 0)
  {
    auto cylIntersector = std::make_shared<raytracing::CylinderIntersector>();
    cylIntersector->SetData(coords, cylExtractor.GetCylIds(), cylExtractor.GetRadii());
    this->Internals->Tracer.AddShapeIntersector(cylIntersector);
    shapeBounds.Include(cylIntersector->GetShapeBounds());
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

void MapperCylinder::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

void MapperCylinder::StartScene()
{
  // Nothing needs to be done.
}

void MapperCylinder::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperCylinder::NewCopy() const
{
  return new vtkm::rendering::MapperCylinder(*this);
}
}
} // vtkm::rendering
