//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/LineRenderer.h>

#include <vtkm/Transform3D.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/LineRendererBatcher.h>

namespace vtkm
{
namespace rendering
{

LineRenderer::LineRenderer(const vtkm::rendering::Canvas* canvas,
                           vtkm::Matrix<vtkm::Float32, 4, 4> transform,
                           vtkm::rendering::LineRendererBatcher* lineBatcher)
  : Canvas(canvas)
  , Transform(transform)
  , LineBatcher(lineBatcher)
{
}

void LineRenderer::RenderLine(const vtkm::Vec2f_64& point0,
                              const vtkm::Vec2f_64& point1,
                              vtkm::Float32 lineWidth,
                              const vtkm::rendering::Color& color)
{
  RenderLine(vtkm::make_Vec(point0[0], point0[1], 0.0),
             vtkm::make_Vec(point1[0], point1[1], 0.0),
             lineWidth,
             color);
}

void LineRenderer::RenderLine(const vtkm::Vec3f_64& point0,
                              const vtkm::Vec3f_64& point1,
                              vtkm::Float32 vtkmNotUsed(lineWidth),
                              const vtkm::rendering::Color& color)
{
  vtkm::Vec3f_32 p0 = TransformPoint(point0);
  vtkm::Vec3f_32 p1 = TransformPoint(point1);
  this->LineBatcher->BatchLine(p0, p1, color);
}

vtkm::Vec3f_32 LineRenderer::TransformPoint(const vtkm::Vec3f_64& point) const
{
  vtkm::Vec4f_32 temp(static_cast<vtkm::Float32>(point[0]),
                      static_cast<vtkm::Float32>(point[1]),
                      static_cast<vtkm::Float32>(point[2]),
                      1.0f);
  temp = vtkm::MatrixMultiply(Transform, temp);
  vtkm::Vec3f_32 p;
  for (vtkm::IdComponent i = 0; i < 3; ++i)
  {
    p[i] = static_cast<vtkm::Float32>(temp[i] / temp[3]);
  }
  p[0] = (p[0] * 0.5f + 0.5f) * static_cast<vtkm::Float32>(Canvas->GetWidth());
  p[1] = (p[1] * 0.5f + 0.5f) * static_cast<vtkm::Float32>(Canvas->GetHeight());
  p[2] = (p[2] * 0.5f + 0.5f) - 0.001f;
  return p;
}
}
} // namespace vtkm::rendering
