//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_View2D_h
#define vtk_m_rendering_View2D_h

#include <vtkm/rendering/View.h>

#include <vtkm/rendering/AxisAnnotation2D.h>
#include <vtkm/rendering/ColorBarAnnotation.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT View2D : public vtkm::rendering::View
{
public:
  View2D(const vtkm::rendering::Scene& scene,
         const vtkm::rendering::Mapper& mapper,
         const vtkm::rendering::Canvas& canvas,
         const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1),
         const vtkm::rendering::Color& foregroundColor = vtkm::rendering::Color(1, 1, 1, 1));

  View2D(const vtkm::rendering::Scene& scene,
         const vtkm::rendering::Mapper& mapper,
         const vtkm::rendering::Canvas& canvas,
         const vtkm::rendering::Camera& camera,
         const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1),
         const vtkm::rendering::Color& foregroundColor = vtkm::rendering::Color(1, 1, 1, 1));

  ~View2D();

  void Paint() override;

  void RenderScreenAnnotations() override;

  void RenderWorldAnnotations() override;

private:
  void UpdateCameraProperties();

  // 2D-specific annotations
  vtkm::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  vtkm::rendering::AxisAnnotation2D VerticalAxisAnnotation;
  vtkm::rendering::ColorBarAnnotation ColorBarAnnotation;
};
}
} // namespace vtkm::rendering

#endif //vtk_m_rendering_View2D_h
