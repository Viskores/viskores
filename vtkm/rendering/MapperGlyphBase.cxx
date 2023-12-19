//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/MapperGlyphBase.h>

#include <vtkm/filter/entity_extraction/MaskPoints.h>
#include <vtkm/rendering/CanvasRayTracer.h>

namespace vtkm
{
namespace rendering
{

MapperGlyphBase::MapperGlyphBase() {}

MapperGlyphBase::~MapperGlyphBase() {}

void MapperGlyphBase::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  vtkm::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<vtkm::rendering::CanvasRayTracer*>(canvas);
  if (canvasRT == nullptr)
  {
    throw vtkm::cont::ErrorBadValue("MapperGlyphBase: bad canvas type. Must be CanvasRayTracer");
  }

  this->Canvas = canvasRT;
}

vtkm::rendering::Canvas* MapperGlyphBase::GetCanvas() const
{
  return this->Canvas;
}

vtkm::cont::Field::Association MapperGlyphBase::GetAssociation() const
{
  return this->Association;
}

void MapperGlyphBase::SetAssociation(vtkm::cont::Field::Association association)
{
  switch (association)
  {
    case vtkm::cont::Field::Association::Cells:
    case vtkm::cont::Field::Association::Points:
      this->Association = association;
      break;
    default:
      throw vtkm::cont::ErrorBadValue("Invalid glyph association.");
  }
}

bool MapperGlyphBase::GetUseCells() const
{
  return this->Association == vtkm::cont::Field::Association::Cells;
}

void MapperGlyphBase::SetUseCells()
{
  this->SetAssociation(vtkm::cont::Field::Association::Cells);
}

bool MapperGlyphBase::GetUsePoints() const
{
  return this->Association == vtkm::cont::Field::Association::Points;
}

void MapperGlyphBase::SetUsePoints()
{
  this->SetAssociation(vtkm::cont::Field::Association::Points);
}

bool MapperGlyphBase::GetUseNodes() const
{
  return this->GetUsePoints();
}

void MapperGlyphBase::SetUseNodes()
{
  this->SetUsePoints();
}

vtkm::Float32 MapperGlyphBase::GetBaseSize() const
{
  return this->BaseSize;
}

void MapperGlyphBase::SetBaseSize(vtkm::Float32 size)
{
  if (size <= 0.f)
  {
    throw vtkm::cont::ErrorBadValue("MapperGlyphBase: base size must be positive");
  }
  this->BaseSize = size;
}

bool MapperGlyphBase::GetScaleByValue() const
{
  return this->ScaleByValue;
}

void MapperGlyphBase::SetScaleByValue(bool on)
{
  this->ScaleByValue = on;
}

vtkm::Float32 MapperGlyphBase::GetScaleDelta() const
{
  return this->ScaleDelta;
}

void MapperGlyphBase::SetScaleDelta(vtkm::Float32 delta)
{
  if (delta < 0.0f)
  {
    throw vtkm::cont::ErrorBadValue("MapperGlyphBase: scale delta must be non-negative");
  }

  this->ScaleDelta = delta;
}

bool MapperGlyphBase::GetUseStride() const
{
  return this->UseStride;
}

void MapperGlyphBase::SetUseStride(bool on)
{
  this->UseStride = on;
}

vtkm::Id MapperGlyphBase::GetStride() const
{
  return this->Stride;
}

void MapperGlyphBase::SetStride(vtkm::Id stride)
{
  if (stride < 1)
  {
    throw vtkm::cont::ErrorBadValue("MapperGlyphBase: stride must be positive");
  }
  this->Stride = stride;
}

void MapperGlyphBase::SetCompositeBackground(bool on)
{
  this->CompositeBackground = on;
}

vtkm::cont::DataSet MapperGlyphBase::FilterPoints(const vtkm::cont::UnknownCellSet& cellSet,
                                                  const vtkm::cont::CoordinateSystem& coords,
                                                  const vtkm::cont::Field& field) const
{
  vtkm::cont::DataSet result;
  result.SetCellSet(cellSet);
  result.AddCoordinateSystem(coords);
  result.AddField(field);

  if (this->UseStride)
  {
    vtkm::filter::entity_extraction::MaskPoints pointMasker;
    pointMasker.SetCompactPoints(true);
    pointMasker.SetStride(this->Stride);
    result = pointMasker.Execute(result);
  }

  return result;
}

}
} // namespace vtkm::rendering
