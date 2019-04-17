//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_View3D_h
#define vtk_m_rendering_View3D_h

#include <vtkm/rendering/View.h>

#include <vtkm/rendering/AxisAnnotation3D.h>
#include <vtkm/rendering/BoundingBoxAnnotation.h>
#include <vtkm/rendering/ColorBarAnnotation.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT View3D : public vtkm::rendering::View
{
public:
  View3D(const vtkm::rendering::Scene& scene,
         const vtkm::rendering::Mapper& mapper,
         const vtkm::rendering::Canvas& canvas,
         const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1),
         const vtkm::rendering::Color& foregroundColor = vtkm::rendering::Color(1, 1, 1, 1));

  View3D(const vtkm::rendering::Scene& scene,
         const vtkm::rendering::Mapper& mapper,
         const vtkm::rendering::Canvas& canvas,
         const vtkm::rendering::Camera& camera,
         const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1),
         const vtkm::rendering::Color& foregroundColor = vtkm::rendering::Color(1, 1, 1, 1));

  ~View3D();

  void Paint() override;

  void RenderScreenAnnotations() override;

  void RenderWorldAnnotations() override;

private:
  // 3D-specific annotations
  vtkm::rendering::BoundingBoxAnnotation BoxAnnotation;
  vtkm::rendering::AxisAnnotation3D XAxisAnnotation;
  vtkm::rendering::AxisAnnotation3D YAxisAnnotation;
  vtkm::rendering::AxisAnnotation3D ZAxisAnnotation;
  vtkm::rendering::ColorBarAnnotation ColorBarAnnotation;
};
}
} // namespace vtkm::rendering

#endif //vtk_m_rendering_View3D_h
