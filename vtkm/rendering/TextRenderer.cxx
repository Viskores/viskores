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
#include <vtkm/rendering/TextRendererBatcher.h>

namespace vtkm
{
namespace rendering
{

TextRenderer::TextRenderer(const vtkm::rendering::Canvas* canvas,
                           const vtkm::rendering::BitmapFont& font,
                           const vtkm::rendering::Canvas::FontTextureType& fontTexture,
                           vtkm::rendering::TextRendererBatcher* textBatcher)
  : Canvas(canvas)
  , Font(font)
  , FontTexture(fontTexture)
  , TextBatcher(textBatcher)
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

  using ScreenCoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id4>;
  using TextureCoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
  ScreenCoordsArrayHandle screenCoords;
  TextureCoordsArrayHandle textureCoords;
  {
    screenCoords.Allocate(static_cast<vtkm::Id>(text.length()));
    textureCoords.Allocate(static_cast<vtkm::Id>(text.length()));
    ScreenCoordsArrayHandle::WritePortalType screenCoordsPortal = screenCoords.WritePortal();
    TextureCoordsArrayHandle::WritePortalType textureCoordsPortal = textureCoords.WritePortal();
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
  }

  this->TextBatcher->BatchText(screenCoords, textureCoords, color, depth);
}
}
} // namespace vtkm::rendering
