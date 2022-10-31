//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperGlyphVector.h>

#include <vtkm/cont/Timer.h>

#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/GlyphExtractorVector.h>
#include <vtkm/rendering/raytracing/GlyphIntersectorVector.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracer.h>
#include <vtkm/rendering/raytracing/SphereExtractor.h>
#include <vtkm/rendering/raytracing/SphereIntersector.h>

namespace vtkm
{
namespace rendering
{

MapperGlyphVector::MapperGlyphVector()
  : MapperGlyphBase()
  , GlyphType(vtkm::rendering::GlyphType::Arrow)
{
}

MapperGlyphVector::~MapperGlyphVector() {}

vtkm::rendering::GlyphType MapperGlyphVector::GetGlyphType() const
{
  return this->GlyphType;
}

void MapperGlyphVector::SetGlyphType(vtkm::rendering::GlyphType glyphType)
{
  if (!(glyphType == vtkm::rendering::GlyphType::Arrow))
  {
    throw vtkm::cont::ErrorBadValue("MapperGlyphVector: bad glyph type");
  }

  this->GlyphType = glyphType;
}

void MapperGlyphVector::RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                                    const vtkm::cont::CoordinateSystem& coords,
                                    const vtkm::cont::Field& field,
                                    const vtkm::cont::ColorTable& vtkmNotUsed(colorTable),
                                    const vtkm::rendering::Camera& camera,
                                    const vtkm::Range& vtkmNotUsed(fieldRange))
{
  raytracing::Logger* logger = raytracing::Logger::GetInstance();

  vtkm::rendering::raytracing::RayTracer tracer;
  tracer.Clear();

  logger->OpenLogEntry("mapper_glyph_vector");
  vtkm::cont::Timer tot_timer;
  tot_timer.Start();
  vtkm::cont::Timer timer;

  vtkm::Bounds coordBounds = coords.GetBounds();
  vtkm::Float32 baseSize = this->BaseSize;
  if (baseSize == -1.f)
  {
    // set a default size
    vtkm::Float64 lx = coordBounds.X.Length();
    vtkm::Float64 ly = coordBounds.Y.Length();
    vtkm::Float64 lz = coordBounds.Z.Length();
    vtkm::Float64 mag = vtkm::Sqrt(lx * lx + ly * ly + lz * lz);
    // same as used in vtk ospray
    constexpr vtkm::Float64 heuristic = 500.;
    baseSize = static_cast<vtkm::Float32>(mag / heuristic);
  }

  vtkm::rendering::raytracing::GlyphExtractorVector glyphExtractor;

  vtkm::cont::DataSet processedDataSet = this->FilterPoints(cellset, coords, field);
  vtkm::cont::UnknownCellSet processedCellSet = processedDataSet.GetCellSet();
  vtkm::cont::CoordinateSystem processedCoords = processedDataSet.GetCoordinateSystem();
  vtkm::cont::Field processedField = processedDataSet.GetField(field.GetName());

  if (this->ScaleByValue)
  {
    vtkm::Float32 minSize = baseSize - baseSize * this->ScaleDelta;
    vtkm::Float32 maxSize = baseSize + baseSize * this->ScaleDelta;
    if (this->UseNodes)
    {
      glyphExtractor.ExtractCoordinates(processedCoords, processedField, minSize, maxSize);
    }
    else
    {
      glyphExtractor.ExtractCells(processedCellSet, processedField, minSize, maxSize);
    }
  }
  else
  {
    if (this->UseNodes)
    {
      glyphExtractor.ExtractCoordinates(processedCoords, processedField, baseSize);
    }
    else
    {
      glyphExtractor.ExtractCells(processedCellSet, processedField, baseSize);
    }
  }

  vtkm::Bounds shapeBounds;
  if (glyphExtractor.GetNumberOfGlyphs() > 0)
  {
    auto glyphIntersector = std::make_shared<raytracing::GlyphIntersectorVector>(this->GlyphType);
    if (this->GlyphType == vtkm::rendering::GlyphType::Arrow)
    {
      vtkm::Float32 arrowBodyRadius = 0.08f * baseSize;
      vtkm::Float32 arrowHeadRadius = 0.16f * baseSize;
      glyphIntersector->SetArrowRadii(arrowBodyRadius, arrowHeadRadius);
    }
    glyphIntersector->SetData(
      processedCoords, glyphExtractor.GetPointIds(), glyphExtractor.GetSizes());

    tracer.AddShapeIntersector(glyphIntersector);
    shapeBounds.Include(glyphIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  vtkm::Int32 width = (vtkm::Int32)this->Canvas->GetWidth();
  vtkm::Int32 height = (vtkm::Int32)this->Canvas->GetHeight();

  vtkm::rendering::raytracing::Camera RayCamera;
  vtkm::rendering::raytracing::Ray<vtkm::Float32> Rays;

  RayCamera.SetParameters(camera, width, height);

  RayCamera.CreateRays(Rays, shapeBounds);
  Rays.Buffers.at(0).InitConst(0.f);
  raytracing::RayOperations::MapCanvasToRays(Rays, camera, *this->Canvas);

  auto magnitudeField = glyphExtractor.GetMagnitudeField();
  auto magnitudeFieldRange = magnitudeField.GetRange().ReadPortal().Get(0);
  tracer.SetField(magnitudeField, magnitudeFieldRange);
  tracer.GetCamera() = RayCamera;
  tracer.SetColorMap(this->ColorMap);
  tracer.Render(Rays);

  timer.Start();
  this->Canvas->WriteToCanvas(Rays, Rays.Buffers.at(0).Buffer, camera);

  if (this->CompositeBackground)
  {
    this->Canvas->BlendBackground();
  }

  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("write_to_canvas", time);
  time = tot_timer.GetElapsedTime();
  logger->CloseLogEntry(time);
}

vtkm::rendering::Mapper* MapperGlyphVector::NewCopy() const
{
  return new vtkm::rendering::MapperGlyphVector(*this);
}
}
} // vtkm::rendering
