//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_rendering_TextAnnotationBillboard_h
#define vtkm_rendering_TextAnnotationBillboard_h

#include <vtkm/rendering/TextAnnotation.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextAnnotationBillboard : public TextAnnotation
{
protected:
  vtkm::Vec3f_32 Position;
  vtkm::Float32 Angle;

public:
  TextAnnotationBillboard(const std::string& text,
                          const vtkm::rendering::Color& color,
                          vtkm::Float32 scalar,
                          const vtkm::Vec3f_32& position,
                          vtkm::Float32 angleDegrees = 0);

  ~TextAnnotationBillboard();

  void SetPosition(const vtkm::Vec3f_32& position);

  void SetPosition(vtkm::Float32 posx, vtkm::Float32 posy, vtkm::Float32 posz);

  void Render(const vtkm::rendering::Camera& camera,
              const vtkm::rendering::WorldAnnotator& worldAnnotator,
              vtkm::rendering::Canvas& canvas) const override;
};
}
} // namespace vtkm::rendering

#endif //vtkm_rendering_TextAnnotationBillboard_h
