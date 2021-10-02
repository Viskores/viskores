//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_TextRendererBatcher_h
#define vtk_m_rendering_TextRendererBatcher_h

#include <string>
#include <vector>

#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextRendererBatcher
{
public:
  using FontTextureType = vtkm::rendering::Canvas::FontTextureType;
  using ScreenCoordsType = vtkm::Id4;
  using TextureCoordsType = vtkm::Vec4f_32;
  using ColorType = vtkm::Vec4f_32;
  using ScreenCoordsArrayHandle = vtkm::cont::ArrayHandle<ScreenCoordsType>;
  using TextureCoordsArrayHandle = vtkm::cont::ArrayHandle<TextureCoordsType>;
  using ColorsArrayHandle = vtkm::cont::ArrayHandle<ColorType>;
  using DepthsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Float32>;

  /*
  VTKM_CONT
  TextRendererBatcher();
  */

  VTKM_CONT
  TextRendererBatcher(const vtkm::rendering::Canvas::FontTextureType& fontTexture);

  VTKM_CONT
  void BatchText(const ScreenCoordsArrayHandle& screenCoords,
                 const TextureCoordsArrayHandle& textureCoords,
                 const vtkm::rendering::Color& color,
                 const vtkm::Float32& depth);

  void Render(const vtkm::rendering::Canvas* canvas) const;

private:
  vtkm::rendering::Canvas::FontTextureType FontTexture;
  std::vector<ScreenCoordsType> ScreenCoords;
  std::vector<TextureCoordsType> TextureCoords;
  std::vector<ColorType> Colors;
  std::vector<vtkm::Float32> Depths;
};
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_TextRendererBatcher_h
