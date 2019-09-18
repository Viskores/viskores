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

void WorldAnnotator::AddLine(const vtkm::Vec3f_64& point0,
                             const vtkm::Vec3f_64& point1,
                             vtkm::Float32 lineWidth,
                             const vtkm::rendering::Color& color,
                             bool vtkmNotUsed(inFront)) const
{
  vtkm::Matrix<vtkm::Float32, 4, 4> transform =
    vtkm::MatrixMultiply(Canvas->GetProjection(), Canvas->GetModelView());
  LineRenderer renderer(Canvas, transform);
  renderer.RenderLine(point0, point1, lineWidth, color);
}

void WorldAnnotator::AddText(const vtkm::Vec3f_32& origin,
                             const vtkm::Vec3f_32& right,
                             const vtkm::Vec3f_32& up,
                             vtkm::Float32 scale,
                             const vtkm::Vec2f_32& anchor,
                             const vtkm::rendering::Color& color,
                             const std::string& text,
                             const vtkm::Float32 depth) const
{
  vtkm::Vec3f_32 n = vtkm::Cross(right, up);
  vtkm::Normalize(n);

  vtkm::Matrix<vtkm::Float32, 4, 4> transform = MatrixHelpers::WorldMatrix(origin, right, up, n);
  transform = vtkm::MatrixMultiply(Canvas->GetModelView(), transform);
  transform = vtkm::MatrixMultiply(Canvas->GetProjection(), transform);
  Canvas->AddText(transform, scale, anchor, color, text, depth);
}
}
} // namespace vtkm::rendering
