//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/TextRendererBatcher.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace
{

struct RenderBitmapFont : public vtkm::worklet::WorkletMapField
{
  using ColorBufferType = vtkm::rendering::Canvas::ColorBufferType;
  using DepthBufferType = vtkm::rendering::Canvas::DepthBufferType;
  using FontTextureType = vtkm::rendering::Canvas::FontTextureType;

  using ControlSignature =
    void(FieldIn, FieldIn, FieldIn, FieldIn, ExecObject, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  using InputDomain = _1;

  VTKM_CONT
  RenderBitmapFont() {}

  VTKM_CONT
  RenderBitmapFont(vtkm::Id width, vtkm::Id height)
    : Width(width)
    , Height(height)
  {
  }

  template <typename ColorBufferPortal, typename FontTexture, typename DepthBufferPortal>
  VTKM_EXEC void operator()(const vtkm::Vec4f_32& screenCoords,
                            const vtkm::Vec4f_32& textureCoords,
                            const vtkm::Vec4f_32& color,
                            const vtkm::Float32& depth,
                            const FontTexture& fontTexture,
                            ColorBufferPortal& colorBuffer,
                            DepthBufferPortal& depthBuffer) const
  {
    vtkm::Float32 x0 = Clamp(screenCoords[0], 0.0f, static_cast<vtkm::Float32>(Width - 1));
    vtkm::Float32 x1 = Clamp(screenCoords[2], 0.0f, static_cast<vtkm::Float32>(Width - 1));
    vtkm::Float32 y0 = Clamp(screenCoords[1], 0.0f, static_cast<vtkm::Float32>(Height - 1));
    vtkm::Float32 y1 = Clamp(screenCoords[3], 0.0f, static_cast<vtkm::Float32>(Height - 1));
    // For crisp text rendering, we sample the font texture at points smaller than the pixel
    // sizes. Here we sample at increments of 0.25f, and scale the reported intensities accordingly
    vtkm::Float32 dx = x1 - x0, dy = y1 - y0;
    for (vtkm::Float32 x = x0; x <= x1; x += 0.25f)
    {
      for (vtkm::Float32 y = y0; y <= y1; y += 0.25f)
      {
        vtkm::Float32 tu = x1 == x0 ? 1.0f : (x - x0) / dx;
        vtkm::Float32 tv = y1 == y0 ? 1.0f : (y - y0) / dy;
        vtkm::Float32 u = vtkm::Lerp(textureCoords[0], textureCoords[2], tu);
        vtkm::Float32 v = vtkm::Lerp(textureCoords[1], textureCoords[3], tv);
        vtkm::Float32 intensity = fontTexture.GetColor(u, v)[0] * 0.25f;
        Plot(x, y, intensity, color, depth, colorBuffer, depthBuffer);
      }
    }
  }

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VTKM_EXEC void Plot(vtkm::Float32 x,
                      vtkm::Float32 y,
                      vtkm::Float32 intensity,
                      vtkm::Vec4f_32 color,
                      vtkm::Float32 depth,
                      ColorBufferPortal& colorBuffer,
                      DepthBufferPortal& depthBuffer) const
  {
    vtkm::Id index =
      static_cast<vtkm::Id>(vtkm::Round(y)) * Width + static_cast<vtkm::Id>(vtkm::Round(x));
    vtkm::Vec4f_32 srcColor = colorBuffer.Get(index);
    vtkm::Float32 currentDepth = depthBuffer.Get(index);
    bool swap = depth > currentDepth;

    intensity = intensity * color[3];
    color = intensity * color;
    color[3] = intensity;
    vtkm::Vec4f_32 front = color;
    vtkm::Vec4f_32 back = srcColor;

    if (swap)
    {
      front = srcColor;
      back = color;
    }

    vtkm::Vec4f_32 blendedColor;
    vtkm::Float32 alpha = (1.f - front[3]);
    blendedColor[0] = front[0] + back[0] * alpha;
    blendedColor[1] = front[1] + back[1] * alpha;
    blendedColor[2] = front[2] + back[2] * alpha;
    blendedColor[3] = back[3] * alpha + front[3];

    colorBuffer.Set(index, blendedColor);
  }

  VTKM_EXEC
  vtkm::Float32 Clamp(vtkm::Float32 v, vtkm::Float32 min, vtkm::Float32 max) const
  {
    return vtkm::Min(vtkm::Max(v, min), max);
  }

  vtkm::Id Width;
  vtkm::Id Height;
}; // struct RenderBitmapFont
} // namespace

TextRendererBatcher::TextRendererBatcher(
  const vtkm::rendering::Canvas::FontTextureType& fontTexture)
  : FontTexture(fontTexture)
{
}

void TextRendererBatcher::BatchText(const ScreenCoordsArrayHandle& screenCoords,
                                    const TextureCoordsArrayHandle& textureCoords,
                                    const vtkm::rendering::Color& color,
                                    const vtkm::Float32& depth)
{
  vtkm::Id textLength = screenCoords.GetNumberOfValues();
  ScreenCoordsArrayHandle::ReadPortalType screenCoordsP = screenCoords.ReadPortal();
  TextureCoordsArrayHandle::ReadPortalType textureCoordsP = textureCoords.ReadPortal();
  for (int i = 0; i < textLength; ++i)
  {
    this->ScreenCoords.push_back(screenCoordsP.Get(i));
    this->TextureCoords.push_back(textureCoordsP.Get(i));
    this->Colors.push_back(color.Components);
    this->Depths.push_back(depth);
  }
}

void TextRendererBatcher::Render(const vtkm::rendering::Canvas* canvas) const
{
  ScreenCoordsArrayHandle screenCoords =
    vtkm::cont::make_ArrayHandle(this->ScreenCoords, vtkm::CopyFlag::Off);
  TextureCoordsArrayHandle textureCoords =
    vtkm::cont::make_ArrayHandle(this->TextureCoords, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayHandle<ColorType> colors =
    vtkm::cont::make_ArrayHandle(this->Colors, vtkm::CopyFlag::Off);
  vtkm::cont::ArrayHandle<vtkm::Float32> depths =
    vtkm::cont::make_ArrayHandle(this->Depths, vtkm::CopyFlag::Off);

  vtkm::cont::Invoker invoker;
  invoker(RenderBitmapFont(canvas->GetWidth(), canvas->GetHeight()),
          screenCoords,
          textureCoords,
          colors,
          depths,
          this->FontTexture.GetExecObjectFactory(),
          canvas->GetColorBuffer(),
          canvas->GetDepthBuffer());
}
}
} // namespace vtkm::rendering
