//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_TextAnnotationScreen_h
#define vtk_m_rendering_TextAnnotationScreen_h

#include <vtkm/rendering/TextAnnotation.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextAnnotationScreen : public TextAnnotation
{
protected:
  vtkm::Vec2f_32 Position;
  vtkm::Float32 Angle;

public:
  TextAnnotationScreen(const std::string& text,
                       const vtkm::rendering::Color& color,
                       vtkm::Float32 scale,
                       const vtkm::Vec2f_32& position,
                       vtkm::Float32 angleDegrees = 0);

  ~TextAnnotationScreen();

  void SetPosition(const vtkm::Vec2f_32& position);

  void SetPosition(vtkm::Float32 posx, vtkm::Float32 posy);

  void Render(const vtkm::rendering::Camera& camera,
              const vtkm::rendering::WorldAnnotator& annotator,
              vtkm::rendering::Canvas& canvas) const override;
};
}
} // namespace vtkm::rendering

#endif //vtk_m_rendering_TextAnnotationScreen_h
