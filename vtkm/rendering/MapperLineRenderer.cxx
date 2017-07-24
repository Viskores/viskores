//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/MapperLineRenderer.h>

namespace vtkm
{
namespace rendering
{

MapperLineRenderer::MapperLineRenderer()
{
}

MapperLineRenderer::~MapperLineRenderer()
{
}

void MapperLineRenderer::SetCanvas(vtkm::rendering::Canvas* canvas)
{
  if (canvas != nullptr)
  {
    this->Canvas = dynamic_cast<vtkm::rendering::CanvasLineRenderer*>(canvas);
    if (this->Canvas == nullptr)
    {
      throw vtkm::cont::ErrorBadValue(
        "LineRenderer Mapper: bad canvas type. Must be a CanvasLineRenderer");
    }
  }
  else
  {
    this->Canvas = nullptr;
  }
}

vtkm::rendering::Canvas* MapperLineRenderer::GetCanvas() const
{
  return this->Canvas;
}

void MapperLineRenderer::RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                                     const vtkm::cont::CoordinateSystem& coords,
                                     const vtkm::cont::Field& scalarField,
                                     const vtkm::rendering::ColorTable& vtkmNotUsed(colorTable),
                                     const vtkm::rendering::Camera& camera,
                                     const vtkm::Range& scalarRange)
{
  //TODO: Extract lines from the input cell set and pass it on to the canvas
}

void MapperLineRenderer::SetActiveColorTable(const ColorTable& ct)
{
}

void MapperLineRenderer::StartScene()
{
  // Nothing needs to be done.
}

void MapperLineRenderer::EndScene()
{
  // Nothing needs to be done.
}

vtkm::rendering::Mapper* MapperLineRenderer::NewCopy() const
{
  return new vtkm::rendering::MapperLineRenderer(*this);
}
}
} // vtkm::rendering
