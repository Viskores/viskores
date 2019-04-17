//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_WorldAnnotatorGL_h
#define vtk_m_rendering_WorldAnnotatorGL_h

#include <vtkm/rendering/WorldAnnotator.h>

#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/TextureGL.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT WorldAnnotatorGL : public WorldAnnotator
{
public:
  WorldAnnotatorGL(const vtkm::rendering::Canvas* canvas);

  ~WorldAnnotatorGL();

  void AddLine(const vtkm::Vec<vtkm::Float64, 3>& point0,
               const vtkm::Vec<vtkm::Float64, 3>& point1,
               vtkm::Float32 lineWidth,
               const vtkm::rendering::Color& color,
               bool inFront) const override;

  void AddText(const vtkm::Vec<vtkm::Float32, 3>& origin,
               const vtkm::Vec<vtkm::Float32, 3>& right,
               const vtkm::Vec<vtkm::Float32, 3>& up,
               vtkm::Float32 scale,
               const vtkm::Vec<vtkm::Float32, 2>& anchor,
               const vtkm::rendering::Color& color,
               const std::string& text,
               const vtkm::Float32 depth = 0.f) const override;

private:
  BitmapFont Font;
  TextureGL FontTexture;

  void RenderText(vtkm::Float32 scale,
                  vtkm::Float32 anchorx,
                  vtkm::Float32 anchory,
                  std::string text) const;
};
}
} //namespace vtkm::rendering

#endif // vtk_m_rendering_WorldAnnotatorGL_h
