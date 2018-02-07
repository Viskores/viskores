//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_rendering_TextRenderer_h
#define vtk_m_rendering_TextRenderer_h

#include <string>

#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextRenderer
{
public:
  VTKM_CONT
  TextRenderer(const vtkm::rendering::Canvas* canvas,
               const vtkm::rendering::BitmapFont& font,
               const vtkm::rendering::Canvas::FontTextureType& fontTexture);

  VTKM_CONT
  void RenderText(const vtkm::Vec<vtkm::Float32, 2>& position,
                  vtkm::Float32 scale,
                  vtkm::Float32 angle,
                  vtkm::Float32 windowAspect,
                  const vtkm::Vec<vtkm::Float32, 2>& anchor,
                  const vtkm::rendering::Color& color,
                  const std::string& text);

  VTKM_CONT
  void RenderText(const vtkm::Vec<vtkm::Float32, 3>& origin,
                  const vtkm::Vec<vtkm::Float32, 3>& right,
                  const vtkm::Vec<vtkm::Float32, 3>& up,
                  vtkm::Float32 scale,
                  const vtkm::Vec<vtkm::Float32, 2>& anchor,
                  const vtkm::rendering::Color& color,
                  const std::string& text);

  VTKM_CONT
  void RenderText(const vtkm::Matrix<vtkm::Float32, 4, 4>& transform,
                  vtkm::Float32 scale,
                  const vtkm::Vec<vtkm::Float32, 2>& anchor,
                  const vtkm::rendering::Color& color,
                  const std::string& text,
                  const vtkm::Float32& depth = 0.f);

private:
  const vtkm::rendering::Canvas* Canvas;
  vtkm::rendering::BitmapFont Font;
  vtkm::rendering::Canvas::FontTextureType FontTexture;
};
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_TextRenderer_h
