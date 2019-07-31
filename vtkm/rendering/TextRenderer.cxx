//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/TextRenderer.h>

#include <vtkm/Transform3D.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace internal
{

struct RenderBitmapFont : public vtkm::worklet::WorkletMapField
{
  using ColorBufferType = vtkm::rendering::Canvas::ColorBufferType;
  using DepthBufferType = vtkm::rendering::Canvas::DepthBufferType;
  using FontTextureType = vtkm::rendering::Canvas::FontTextureType;

  using ControlSignature = void(FieldIn, FieldIn, ExecObject, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  VTKM_CONT
  RenderBitmapFont() {}

  VTKM_CONT
  RenderBitmapFont(const vtkm::Vec4f_32& color,
                   vtkm::Id width,
                   vtkm::Id height,
                   vtkm::Float32 depth)
    : Color(color)
    , Width(width)
    , Height(height)
    , Depth(depth)
  {
  }

  template <typename ColorBufferPortal, typename FontTexture, typename DepthBufferPortal>
  VTKM_EXEC void operator()(const vtkm::Vec4f_32& screenCoords,
                            const vtkm::Vec4f_32& textureCoords,
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
        Plot(x, y, intensity, colorBuffer, depthBuffer);
      }
    }
  }

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VTKM_EXEC void Plot(vtkm::Float32 x,
                      vtkm::Float32 y,
                      vtkm::Float32 intensity,
                      ColorBufferPortal& colorBuffer,
                      DepthBufferPortal& depthBuffer) const
  {
    vtkm::Id index =
      static_cast<vtkm::Id>(vtkm::Round(y)) * Width + static_cast<vtkm::Id>(vtkm::Round(x));
    vtkm::Vec4f_32 srcColor = colorBuffer.Get(index);
    vtkm::Float32 currentDepth = depthBuffer.Get(index);
    bool swap = Depth > currentDepth;

    intensity = intensity * Color[3];
    vtkm::Vec4f_32 color = intensity * Color;
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

  vtkm::Vec4f_32 Color;
  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::Float32 Depth;
}; // struct RenderBitmapFont

struct RenderBitmapFontExecutor
{
  using ColorBufferType = vtkm::rendering::Canvas::ColorBufferType;
  using DepthBufferType = vtkm::rendering::Canvas::DepthBufferType;
  using FontTextureType = vtkm::rendering::Canvas::FontTextureType;
  using ScreenCoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id4>;
  using TextureCoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;

  VTKM_CONT
  RenderBitmapFontExecutor(const ScreenCoordsArrayHandle& screenCoords,
                           const TextureCoordsArrayHandle& textureCoords,
                           const FontTextureType& fontTexture,
                           const vtkm::Vec4f_32& color,
                           const ColorBufferType& colorBuffer,
                           const DepthBufferType& depthBuffer,
                           vtkm::Id width,
                           vtkm::Id height,
                           vtkm::Float32 depth)
    : ScreenCoords(screenCoords)
    , TextureCoords(textureCoords)
    , FontTexture(fontTexture)
    , ColorBuffer(colorBuffer)
    , DepthBuffer(depthBuffer)
    , Worklet(color, width, height, depth)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::worklet::DispatcherMapField<RenderBitmapFont> dispatcher(Worklet);
    dispatcher.SetDevice(Device());
    dispatcher.Invoke(
      ScreenCoords, TextureCoords, FontTexture.GetExecObjectFactory(), ColorBuffer, DepthBuffer);
    return true;
  }

  ScreenCoordsArrayHandle ScreenCoords;
  TextureCoordsArrayHandle TextureCoords;
  FontTextureType FontTexture;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
  RenderBitmapFont Worklet;
}; // struct RenderBitmapFontExecutor
} // namespace internal

TextRenderer::TextRenderer(const vtkm::rendering::Canvas* canvas,
                           const vtkm::rendering::BitmapFont& font,
                           const vtkm::rendering::Canvas::FontTextureType& fontTexture)
  : Canvas(canvas)
  , Font(font)
  , FontTexture(fontTexture)
{
}

void TextRenderer::RenderText(const vtkm::Vec2f_32& position,
                              vtkm::Float32 scale,
                              vtkm::Float32 angle,
                              vtkm::Float32 windowAspect,
                              const vtkm::Vec2f_32& anchor,
                              const vtkm::rendering::Color& color,
                              const std::string& text)
{
  vtkm::Matrix<vtkm::Float32, 4, 4> translationMatrix =
    Transform3DTranslate(position[0], position[1], 0.f);
  vtkm::Matrix<vtkm::Float32, 4, 4> scaleMatrix = Transform3DScale(1.0f / windowAspect, 1.0f, 1.0f);
  vtkm::Vec3f_32 rotationAxis(0.0f, 0.0f, 1.0f);
  vtkm::Matrix<vtkm::Float32, 4, 4> rotationMatrix = Transform3DRotate(angle, rotationAxis);
  vtkm::Matrix<vtkm::Float32, 4, 4> transform =
    vtkm::MatrixMultiply(translationMatrix, vtkm::MatrixMultiply(scaleMatrix, rotationMatrix));
  RenderText(transform, scale, anchor, color, text);
}

void TextRenderer::RenderText(const vtkm::Vec3f_32& origin,
                              const vtkm::Vec3f_32& right,
                              const vtkm::Vec3f_32& up,
                              vtkm::Float32 scale,
                              const vtkm::Vec2f_32& anchor,
                              const vtkm::rendering::Color& color,
                              const std::string& text)
{
  vtkm::Vec3f_32 n = vtkm::Cross(right, up);
  vtkm::Normalize(n);

  vtkm::Matrix<vtkm::Float32, 4, 4> transform = MatrixHelpers::WorldMatrix(origin, right, up, n);
  transform = vtkm::MatrixMultiply(Canvas->GetModelView(), transform);
  transform = vtkm::MatrixMultiply(Canvas->GetProjection(), transform);
  RenderText(transform, scale, anchor, color, text);
}

void TextRenderer::RenderText(const vtkm::Matrix<vtkm::Float32, 4, 4>& transform,
                              vtkm::Float32 scale,
                              const vtkm::Vec2f_32& anchor,
                              const vtkm::rendering::Color& color,
                              const std::string& text,
                              const vtkm::Float32& depth)
{
  vtkm::Float32 textWidth = this->Font.GetTextWidth(text);
  vtkm::Float32 fx = -(0.5f + 0.5f * anchor[0]) * textWidth;
  vtkm::Float32 fy = -(0.5f + 0.5f * anchor[1]);
  vtkm::Float32 fz = 0;

  using ScreenCoordsArrayHandle = internal::RenderBitmapFontExecutor::ScreenCoordsArrayHandle;
  using TextureCoordsArrayHandle = internal::RenderBitmapFontExecutor::TextureCoordsArrayHandle;
  ScreenCoordsArrayHandle screenCoords;
  TextureCoordsArrayHandle textureCoords;
  screenCoords.Allocate(static_cast<vtkm::Id>(text.length()));
  textureCoords.Allocate(static_cast<vtkm::Id>(text.length()));
  ScreenCoordsArrayHandle::PortalControl screenCoordsPortal = screenCoords.GetPortalControl();
  TextureCoordsArrayHandle::PortalControl textureCoordsPortal = textureCoords.GetPortalControl();
  vtkm::Vec4f_32 charVertices, charUVs, charCoords;
  for (std::size_t i = 0; i < text.length(); ++i)
  {
    char c = text[i];
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;
    Font.GetCharPolygon(c,
                        fx,
                        fy,
                        charVertices[0],
                        charVertices[2],
                        charVertices[3],
                        charVertices[1],
                        charUVs[0],
                        charUVs[2],
                        charUVs[3],
                        charUVs[1],
                        nextchar);
    charVertices = charVertices * scale;
    vtkm::Id2 p0 = Canvas->GetScreenPoint(charVertices[0], charVertices[3], fz, transform);
    vtkm::Id2 p1 = Canvas->GetScreenPoint(charVertices[2], charVertices[1], fz, transform);
    charCoords = vtkm::Id4(p0[0], p1[1], p1[0], p0[1]);
    screenCoordsPortal.Set(static_cast<vtkm::Id>(i), charCoords);
    textureCoordsPortal.Set(static_cast<vtkm::Id>(i), charUVs);
  }

  vtkm::cont::TryExecute(internal::RenderBitmapFontExecutor(screenCoords,
                                                            textureCoords,
                                                            FontTexture,
                                                            color.Components,
                                                            Canvas->GetColorBuffer(),
                                                            Canvas->GetDepthBuffer(),
                                                            Canvas->GetWidth(),
                                                            Canvas->GetHeight(),
                                                            depth));
}
}
} // namespace vtkm::rendering
