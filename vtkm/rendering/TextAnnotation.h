//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_TextAnnotation_h
#define vtk_m_rendering_TextAnnotation_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextAnnotation
{
public:
  enum HorizontalAlignment
  {
    Left,
    HCenter,
    Right
  };
  enum VerticalAlignment
  {
    Bottom,
    VCenter,
    Top
  };

protected:
  std::string Text;
  Color TextColor;
  vtkm::Float32 Scale;
  vtkm::Vec2f_32 Anchor;

public:
  TextAnnotation(const std::string& text,
                 const vtkm::rendering::Color& color,
                 vtkm::Float32 scalar);

  virtual ~TextAnnotation();

  void SetText(const std::string& text);

  const std::string& GetText() const;

  /// Set the anchor point relative to the box containing the text. The anchor
  /// is scaled in both directions to the range [-1,1] with -1 at the lower
  /// left and 1 at the upper right.
  ///
  void SetRawAnchor(const vtkm::Vec2f_32& anchor);

  void SetRawAnchor(vtkm::Float32 h, vtkm::Float32 v);

  void SetAlignment(HorizontalAlignment h, VerticalAlignment v);

  void SetScale(vtkm::Float32 scale);

  virtual void Render(const vtkm::rendering::Camera& camera,
                      const vtkm::rendering::WorldAnnotator& worldAnnotator,
                      vtkm::rendering::Canvas& canvas) const = 0;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_TextAnnotation_h
