//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/CanvasRayTracer.h>

#include <vtkm/rendering/ConnectivityProxy.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/MapperConnectivity.h>
#include <vtkm/rendering/View.h>

#include <cstdlib>
#include <vtkm/rendering/raytracing/Camera.h>

namespace vtkm
{
namespace rendering
{

VTKM_CONT
MapperConnectivity::MapperConnectivity()
{
  CanvasRT = nullptr;
  SampleDistance = -1;
}

VTKM_CONT
MapperConnectivity::~MapperConnectivity()
{
}

VTKM_CONT
void MapperConnectivity::SetSampleDistance(const vtkm::Float32& distance)
{
  SampleDistance = distance;
}

VTKM_CONT
void MapperConnectivity::SetCanvas(Canvas* canvas)
{
  if (canvas != nullptr)
  {

    CanvasRT = dynamic_cast<CanvasRayTracer*>(canvas);
    if (CanvasRT == nullptr)
    {
      throw vtkm::cont::ErrorBadValue("Volume Render: bad canvas type. Must be CanvasRayTracer");
    }
  }
}

vtkm::rendering::Canvas* MapperConnectivity::GetCanvas() const
{
  return CanvasRT;
}


VTKM_CONT
void MapperConnectivity::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                                     const vtkm::cont::CoordinateSystem& coords,
                                     const vtkm::cont::Field& scalarField,
                                     const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                     const vtkm::rendering::Camera& camera,
                                     const vtkm::Range& vtkmNotUsed(scalarRange))
{
  vtkm::rendering::ConnectivityProxy tracerProxy(cellset, coords, scalarField);
  if (SampleDistance == -1.f)
  {
    // set a default distance
    vtkm::Bounds bounds = coords.GetBounds();
    vtkm::Float64 x2 = bounds.X.Length() * bounds.X.Length();
    vtkm::Float64 y2 = bounds.Y.Length() * bounds.Y.Length();
    vtkm::Float64 z2 = bounds.Z.Length() * bounds.Z.Length();
    vtkm::Float64 length = vtkm::Sqrt(x2 + y2 + z2);
    constexpr vtkm::Float64 defaultSamples = 200.;
    SampleDistance = static_cast<vtkm::Float32>(length / defaultSamples);
  }
  tracerProxy.SetSampleDistance(SampleDistance);
  tracerProxy.SetColorMap(ColorMap);
  tracerProxy.Trace(camera, CanvasRT);
}

void MapperConnectivity::StartScene()
{
  // Nothing needs to be done.
}

void MapperConnectivity::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperConnectivity::NewCopy() const
{
  return new vtkm::rendering::MapperConnectivity(*this);
}
}
} // namespace vtkm::rendering
