//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_LineRenderer_h
#define vtk_m_rendering_LineRenderer_h

#include <vtkm/Matrix.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT LineRenderer
{
public:
  VTKM_CONT
  LineRenderer(const vtkm::rendering::Canvas* canvas, vtkm::Matrix<vtkm::Float32, 4, 4> transform);

  VTKM_CONT
  void RenderLine(const vtkm::Vec2f_64& point0,
                  const vtkm::Vec2f_64& point1,
                  vtkm::Float32 lineWidth,
                  const vtkm::rendering::Color& color);

  VTKM_CONT
  void RenderLine(const vtkm::Vec3f_64& point0,
                  const vtkm::Vec3f_64& point1,
                  vtkm::Float32 lineWidth,
                  const vtkm::rendering::Color& color);

private:
  VTKM_CONT
  vtkm::Vec3f_32 TransformPoint(const vtkm::Vec3f_64& point) const;

  const vtkm::rendering::Canvas* Canvas;
  vtkm::Matrix<vtkm::Float32, 4, 4> Transform;
}; // class LineRenderer
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_LineRenderer_h
