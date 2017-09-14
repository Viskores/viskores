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

#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/DecodePNG.h>
#include <vtkm/rendering/LineRenderer.h>
#include <vtkm/rendering/TextRenderer.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm
{
namespace rendering
{

WorldAnnotator::WorldAnnotator(const vtkm::rendering::Canvas* canvas)
  : Canvas(canvas)
{
}

WorldAnnotator::~WorldAnnotator()
{
}

void WorldAnnotator::AddLine(const vtkm::Vec<vtkm::Float64, 3>& point0,
                             const vtkm::Vec<vtkm::Float64, 3>& point1,
                             vtkm::Float32 lineWidth,
                             const vtkm::rendering::Color& color,
                             bool vtkmNotUsed(inFront)) const
{
  // Default implementation does nothing. Should this be pure virtual and force
  // all subclasses to implement this? We would have to implement a
  // WorldAnnotator for ray tracing first.
  vtkm::Matrix<vtkm::Float32, 4, 4> transform =
    vtkm::MatrixMultiply(Canvas->Projection, Canvas->ModelView);
  LineRenderer renderer(Canvas, transform);
  renderer.RenderLine(point0, point1, lineWidth, color);
}

void WorldAnnotator::AddText(const vtkm::Vec<vtkm::Float32, 3>& origin,
                             const vtkm::Vec<vtkm::Float32, 3>& right,
                             const vtkm::Vec<vtkm::Float32, 3>& up,
                             vtkm::Float32 scale,
                             const vtkm::Vec<vtkm::Float32, 2>& anchor,
                             const vtkm::rendering::Color& color,
                             const std::string& text) const
{
  // Default implementation does nothing. Should this be pure virtual and force
  // all subclasses to implement this? We would have to implement a
  // WorldAnnotator for ray tracing first.
  if (!FontTexture.IsValid())
  {
    if (!LoadFont())
    {
      return;
    }
  }
  TextRenderer renderer(Canvas, Font, FontTexture);
  renderer.RenderText(origin, right, up, scale, anchor, color, text);
}

bool WorldAnnotator::LoadFont() const
{
  this->Font = BitmapFontFactory::CreateLiberation2Sans();
  const std::vector<unsigned char>& rawPNG = this->Font.GetRawImageData();
  std::vector<unsigned char> rgba;
  unsigned long textureWidth, textureHeight;
  int error = DecodePNG(rgba, textureWidth, textureHeight, &rawPNG[0], rawPNG.size());
  if (error != 0)
  {
    return false;
  }
  std::size_t numValues = textureWidth * textureHeight;
  std::vector<unsigned char> alpha(numValues);
  for (std::size_t i = 0; i < numValues; ++i)
  {
    alpha[i] = rgba[i * 4 + 3];
  }
  vtkm::cont::ArrayHandle<vtkm::UInt8> textureHandle = vtkm::cont::make_ArrayHandle(alpha);
  this->FontTexture = vtkm::rendering::Canvas::FontTextureType(
    vtkm::Id(textureWidth), vtkm::Id(textureHeight), textureHandle);
  this->FontTexture.SetFilterMode(TextureFilterMode::Linear);
  this->FontTexture.SetWrapMode(TextureWrapMode::Clamp);
  return true;
}
}
} // namespace vtkm::rendering
