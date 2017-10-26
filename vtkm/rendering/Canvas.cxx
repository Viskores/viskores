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

#include <vtkm/rendering/Canvas.h>

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/DecodePNG.h>
#include <vtkm/rendering/LineRenderer.h>
#include <vtkm/rendering/TextRenderer.h>
#include <vtkm/rendering/WorldAnnotator.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <fstream>
#include <iostream>

namespace vtkm
{
namespace rendering
{
namespace internal
{

struct ClearBuffers : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldOut<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);

  VTKM_CONT
  ClearBuffers() {}

  VTKM_EXEC
  void operator()(vtkm::Vec<vtkm::Float32, 4>& color, vtkm::Float32& depth) const
  {
    color[0] = 0.f;
    color[1] = 0.f;
    color[2] = 0.f;
    color[3] = 0.f;
    // The depth is set to slightly larger than 1.0f, ensuring this color value always fails a
    // depth check
    depth = 1.001f;
  }
}; // struct ClearBuffers

struct ClearBuffersExecutor
{
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;
  typedef vtkm::rendering::Canvas::DepthBufferType DepthBufferType;

  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;

  VTKM_CONT
  ClearBuffersExecutor(const ColorBufferType& colorBuffer, const DepthBufferType& depthBuffer)
    : ColorBuffer(colorBuffer)
    , DepthBuffer(depthBuffer)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    ClearBuffers worklet;
    vtkm::worklet::DispatcherMapField<ClearBuffers, Device> dispatcher(worklet);
    dispatcher.Invoke(this->ColorBuffer, this->DepthBuffer);
    return true;
  }
}; // struct ClearBuffersExecutor

struct BlendBackground : public vtkm::worklet::WorkletMapField
{
  vtkm::Vec<vtkm::Float32, 4> BackgroundColor;

  VTKM_CONT
  BlendBackground(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor)
    : BackgroundColor(backgroundColor)
  {
  }

  typedef void ControlSignature(FieldInOut<>);
  typedef void ExecutionSignature(_1);

  VTKM_EXEC void operator()(vtkm::Vec<vtkm::Float32, 4>& color) const
  {
    if (color[3] >= 1.f)
      return;

    vtkm::Float32 alpha = BackgroundColor[3] * (1.f - color[3]);
    color[0] = color[0] + BackgroundColor[0] * alpha;
    color[1] = color[1] + BackgroundColor[1] * alpha;
    color[2] = color[2] + BackgroundColor[2] * alpha;
    color[3] = alpha + color[3];
  }
}; // struct BlendBackground

struct BlendBackgroundExecutor
{
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;

  ColorBufferType ColorBuffer;
  BlendBackground Worklet;

  VTKM_CONT
  BlendBackgroundExecutor(const ColorBufferType& colorBuffer,
                          const vtkm::Vec<vtkm::Float32, 4>& backgroundColor)
    : ColorBuffer(colorBuffer)
    , Worklet(backgroundColor)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::worklet::DispatcherMapField<BlendBackground, Device> dispatcher(this->Worklet);
    dispatcher.Invoke(this->ColorBuffer);
    return true;
  }
}; // struct BlendBackgroundExecutor

struct DrawColorSwatch : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, WholeArrayInOut<>);
  typedef void ExecutionSignature(_1, _2);

  VTKM_CONT
  DrawColorSwatch(vtkm::Id2 dims,
                  vtkm::Id2 xBounds,
                  vtkm::Id2 yBounds,
                  const vtkm::Vec<vtkm::Float32, 4> color)
    : Color(color)
  {
    ImageWidth = dims[0];
    ImageHeight = dims[1];
    SwatchBottomLeft[0] = xBounds[0];
    SwatchBottomLeft[1] = yBounds[0];
    SwatchWidth = xBounds[1] - xBounds[0];
    SwatchHeight = yBounds[1] - yBounds[0];
  }

  template <typename FrameBuffer>
  VTKM_EXEC void operator()(const vtkm::Id& index, FrameBuffer& frameBuffer) const
  {
    // local bar coord
    vtkm::Id x = index % SwatchWidth;
    vtkm::Id y = index / SwatchWidth;

    // offset to global image coord
    x += SwatchBottomLeft[0];
    y += SwatchBottomLeft[1];

    vtkm::Id offset = y * ImageWidth + x;
    frameBuffer.Set(offset, Color);
  }

  vtkm::Id ImageWidth;
  vtkm::Id ImageHeight;
  vtkm::Id2 SwatchBottomLeft;
  vtkm::Id SwatchWidth;
  vtkm::Id SwatchHeight;
  const vtkm::Vec<vtkm::Float32, 4> Color;
}; // struct DrawColorSwatch

struct DrawColorBar : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, WholeArrayInOut<>, WholeArrayIn<>);
  typedef void ExecutionSignature(_1, _2, _3);

  VTKM_CONT
  DrawColorBar(vtkm::Id2 dims, vtkm::Id2 xBounds, vtkm::Id2 yBounds, bool horizontal)
    : Horizontal(horizontal)
  {
    ImageWidth = dims[0];
    ImageHeight = dims[1];
    BarBottomLeft[0] = xBounds[0];
    BarBottomLeft[1] = yBounds[0];
    BarWidth = xBounds[1] - xBounds[0];
    BarHeight = yBounds[1] - yBounds[0];
  }

  template <typename FrameBuffer, typename ColorMap>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            FrameBuffer& frameBuffer,
                            const ColorMap& colorMap) const
  {
    // local bar coord
    vtkm::Id x = index % BarWidth;
    vtkm::Id y = index / BarWidth, yLocal = y;
    vtkm::Id sample = Horizontal ? x : y;
    vtkm::Vec<vtkm::Float32, 4> color = colorMap.Get(sample);

    // offset to global image coord
    x += BarBottomLeft[0];
    y += BarBottomLeft[1];

    vtkm::Id offset = y * ImageWidth + x;
    // If the colortable has alpha values, we blend each color sample with translucent white.
    // The height of the resultant translucent bar indicates the opacity.
    vtkm::Float32 normalizedHeight = static_cast<vtkm::Float32>(yLocal) / BarHeight;
    if (color[3] < 1.0f && normalizedHeight <= color[3])
    {
      vtkm::Float32 intensity = 0.4f;
      vtkm::Vec<vtkm::Float32, 4> blendedColor;
      vtkm::Vec<vtkm::Float32, 4> srcColor = color;
      vtkm::Float32 inverseIntensity = (1.0f - intensity);
      vtkm::Float32 alpha = srcColor[3] * inverseIntensity;
      blendedColor[0] = 1.0f * intensity + srcColor[0] * alpha;
      blendedColor[1] = 1.0f * intensity + srcColor[1] * alpha;
      blendedColor[2] = 1.0f * intensity + srcColor[2] * alpha;
      blendedColor[3] = 1.0f;
      frameBuffer.Set(offset, blendedColor);
    }
    else
    {
      frameBuffer.Set(offset, color);
    }
  }

  vtkm::Id ImageWidth;
  vtkm::Id ImageHeight;
  vtkm::Id2 BarBottomLeft;
  vtkm::Id BarWidth;
  vtkm::Id BarHeight;
  bool Horizontal;
}; // struct DrawColorBar

struct ColorSwatchExecutor
{
  VTKM_CONT
  ColorSwatchExecutor(vtkm::Id2 dims,
                      vtkm::Id2 xBounds,
                      vtkm::Id2 yBounds,
                      const vtkm::Vec<vtkm::Float32, 4>& color,
                      const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorBuffer)
    : Dims(dims)
    , XBounds(xBounds)
    , YBounds(yBounds)
    , Color(color)
    , ColorBuffer(colorBuffer)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::Id totalPixels = (XBounds[1] - XBounds[0]) * (YBounds[1] - YBounds[0]);
    vtkm::cont::ArrayHandleCounting<vtkm::Id> iterator(0, 1, totalPixels);
    vtkm::worklet::DispatcherMapField<DrawColorSwatch, Device> dispatcher(
      DrawColorSwatch(this->Dims, this->XBounds, this->YBounds, Color));
    dispatcher.Invoke(iterator, this->ColorBuffer);
    return true;
  }

  vtkm::Id2 Dims;
  vtkm::Id2 XBounds;
  vtkm::Id2 YBounds;
  const vtkm::Vec<vtkm::Float32, 4>& Color;
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& ColorBuffer;
}; // struct ColorSwatchExecutor

struct ColorBarExecutor
{
  VTKM_CONT
  ColorBarExecutor(vtkm::Id2 dims,
                   vtkm::Id2 xBounds,
                   vtkm::Id2 yBounds,
                   bool horizontal,
                   vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap,
                   const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorBuffer)
    : Dims(dims)
    , XBounds(xBounds)
    , YBounds(yBounds)
    , Horizontal(horizontal)
    , ColorMap(colorMap)
    , ColorBuffer(colorBuffer)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::Id totalPixels = (XBounds[1] - XBounds[0]) * (YBounds[1] - YBounds[0]);
    vtkm::cont::ArrayHandleCounting<vtkm::Id> iterator(0, 1, totalPixels);
    vtkm::worklet::DispatcherMapField<DrawColorBar, Device> dispatcher(
      DrawColorBar(this->Dims, this->XBounds, this->YBounds, this->Horizontal));
    dispatcher.Invoke(iterator, this->ColorBuffer, this->ColorMap);
    return true;
  }

  vtkm::Id2 Dims;
  vtkm::Id2 XBounds;
  vtkm::Id2 YBounds;
  bool Horizontal;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& ColorMap;
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& ColorBuffer;
}; // struct ColorSwatchExecutor

} // namespace internal

struct Canvas::CanvasInternals
{

  CanvasInternals(vtkm::Id width, vtkm::Id height)
    : Width(width)
    , Height(height)
  {
  }

  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::rendering::Color BackgroundColor;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;
  vtkm::rendering::BitmapFont Font;
  FontTextureType FontTexture;
  vtkm::Matrix<vtkm::Float32, 4, 4> ModelView;
  vtkm::Matrix<vtkm::Float32, 4, 4> Projection;
};

Canvas::Canvas(vtkm::Id width, vtkm::Id height)
  : Internals(new CanvasInternals(0, 0))
{
  vtkm::MatrixIdentity(Internals->ModelView);
  vtkm::MatrixIdentity(Internals->Projection);
  this->ResizeBuffers(width, height);
}

Canvas::~Canvas()
{
}

vtkm::rendering::Canvas* Canvas::NewCopy() const
{
  return new vtkm::rendering::Canvas(*this);
}

vtkm::Id Canvas::GetWidth() const
{
  return Internals->Width;
}

vtkm::Id Canvas::GetHeight() const
{
  return Internals->Height;
}

const Canvas::ColorBufferType& Canvas::GetColorBuffer() const
{
  return Internals->ColorBuffer;
}

Canvas::ColorBufferType& Canvas::GetColorBuffer()
{
  return Internals->ColorBuffer;
}

const Canvas::DepthBufferType& Canvas::GetDepthBuffer() const
{
  return Internals->DepthBuffer;
}

Canvas::DepthBufferType& Canvas::GetDepthBuffer()
{
  return Internals->DepthBuffer;
}

const vtkm::rendering::Color& Canvas::GetBackgroundColor() const
{
  return Internals->BackgroundColor;
}

void Canvas::SetBackgroundColor(const vtkm::rendering::Color& color)
{
  Internals->BackgroundColor = color;
}

void Canvas::Initialize()
{
}

void Canvas::Activate()
{
}

void Canvas::Clear()
{
  // TODO: Should the rendering library support policies or some other way to
  // configure with custom devices?
  vtkm::cont::TryExecute(internal::ClearBuffersExecutor(GetColorBuffer(), GetDepthBuffer()));
}

void Canvas::Finish()
{
}

void Canvas::BlendBackground()
{
  vtkm::cont::TryExecute(
    internal::BlendBackgroundExecutor(GetColorBuffer(), GetBackgroundColor().Components));
}

void Canvas::ResizeBuffers(vtkm::Id width, vtkm::Id height)
{
  VTKM_ASSERT(width >= 0);
  VTKM_ASSERT(height >= 0);

  vtkm::Id numPixels = width * height;
  if (Internals->ColorBuffer.GetNumberOfValues() != numPixels)
  {
    Internals->ColorBuffer.Allocate(numPixels);
  }
  if (Internals->DepthBuffer.GetNumberOfValues() != numPixels)
  {
    Internals->DepthBuffer.Allocate(numPixels);
  }

  Internals->Width = width;
  Internals->Height = height;
}

void Canvas::AddColorSwatch(const vtkm::Vec<vtkm::Float64, 2>& point0,
                            const vtkm::Vec<vtkm::Float64, 2>& vtkmNotUsed(point1),
                            const vtkm::Vec<vtkm::Float64, 2>& point2,
                            const vtkm::Vec<vtkm::Float64, 2>& vtkmNotUsed(point3),
                            const vtkm::rendering::Color& color) const
{
  vtkm::Float64 width = static_cast<vtkm::Float64>(this->GetWidth());
  vtkm::Float64 height = static_cast<vtkm::Float64>(this->GetHeight());

  vtkm::Id2 x, y;
  x[0] = static_cast<vtkm::Id>(((point0[0] + 1.) / 2.) * width + .5);
  x[1] = static_cast<vtkm::Id>(((point2[0] + 1.) / 2.) * width + .5);
  y[0] = static_cast<vtkm::Id>(((point0[1] + 1.) / 2.) * height + .5);
  y[1] = static_cast<vtkm::Id>(((point2[1] + 1.) / 2.) * height + .5);

  vtkm::Id2 dims(this->GetWidth(), this->GetHeight());
  vtkm::cont::TryExecute(
    internal::ColorSwatchExecutor(dims, x, y, color.Components, this->GetColorBuffer()));
}

void Canvas::AddColorSwatch(const vtkm::Float64 x0,
                            const vtkm::Float64 y0,
                            const vtkm::Float64 x1,
                            const vtkm::Float64 y1,
                            const vtkm::Float64 x2,
                            const vtkm::Float64 y2,
                            const vtkm::Float64 x3,
                            const vtkm::Float64 y3,
                            const vtkm::rendering::Color& color) const
{
  this->AddColorSwatch(vtkm::make_Vec(x0, y0),
                       vtkm::make_Vec(x1, y1),
                       vtkm::make_Vec(x2, y2),
                       vtkm::make_Vec(x3, y3),
                       color);
}

void Canvas::AddLine(const vtkm::Vec<vtkm::Float64, 2>& point0,
                     const vtkm::Vec<vtkm::Float64, 2>& point1,
                     vtkm::Float32 linewidth,
                     const vtkm::rendering::Color& color) const
{
  vtkm::rendering::Canvas* self = const_cast<vtkm::rendering::Canvas*>(this);
  LineRenderer renderer(self, vtkm::MatrixMultiply(Internals->Projection, Internals->ModelView));
  renderer.RenderLine(point0, point1, linewidth, color);
}

void Canvas::AddLine(vtkm::Float64 x0,
                     vtkm::Float64 y0,
                     vtkm::Float64 x1,
                     vtkm::Float64 y1,
                     vtkm::Float32 linewidth,
                     const vtkm::rendering::Color& color) const
{
  this->AddLine(vtkm::make_Vec(x0, y0), vtkm::make_Vec(x1, y1), linewidth, color);
}

void Canvas::AddColorBar(const vtkm::Bounds& bounds,
                         const vtkm::rendering::ColorTable& colorTable,
                         bool horizontal) const
{
  vtkm::Float64 width = static_cast<vtkm::Float64>(this->GetWidth());
  vtkm::Float64 height = static_cast<vtkm::Float64>(this->GetHeight());

  vtkm::Id2 x, y;
  x[0] = static_cast<vtkm::Id>(((bounds.X.Min + 1.) / 2.) * width + .5);
  x[1] = static_cast<vtkm::Id>(((bounds.X.Max + 1.) / 2.) * width + .5);
  y[0] = static_cast<vtkm::Id>(((bounds.Y.Min + 1.) / 2.) * height + .5);
  y[1] = static_cast<vtkm::Id>(((bounds.Y.Max + 1.) / 2.) * height + .5);
  vtkm::Id barWidth = x[1] - x[0];
  vtkm::Id barHeight = y[1] - y[0];

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> colorMap;
  vtkm::Id numSamples = horizontal ? barWidth : barHeight;
  colorTable.Sample(static_cast<vtkm::Int32>(numSamples), colorMap);

  vtkm::Id2 dims(this->GetWidth(), this->GetHeight());
  vtkm::cont::TryExecute(
    internal::ColorBarExecutor(dims, x, y, horizontal, colorMap, this->GetColorBuffer()));
}

void Canvas::AddColorBar(vtkm::Float32 x,
                         vtkm::Float32 y,
                         vtkm::Float32 width,
                         vtkm::Float32 height,
                         const vtkm::rendering::ColorTable& colorTable,
                         bool horizontal) const
{
  this->AddColorBar(
    vtkm::Bounds(vtkm::Range(x, x + width), vtkm::Range(y, y + height), vtkm::Range(0, 0)),
    colorTable,
    horizontal);
}

vtkm::Id2 Canvas::GetScreenPoint(vtkm::Float32 x,
                                 vtkm::Float32 y,
                                 vtkm::Float32 z,
                                 const vtkm::Matrix<vtkm::Float32, 4, 4>& transform) const
{
  vtkm::Vec<vtkm::Float32, 4> point(x, y, z, 1.0f);
  point = vtkm::MatrixMultiply(transform, point);

  vtkm::Id2 pixelPos;
  vtkm::Float32 width = static_cast<vtkm::Float32>(Internals->Width);
  vtkm::Float32 height = static_cast<vtkm::Float32>(Internals->Height);
  pixelPos[0] = static_cast<vtkm::Id>(vtkm::Round((1.0f + point[0]) * width * 0.5f + 0.5f));
  pixelPos[1] = static_cast<vtkm::Id>(vtkm::Round((1.0f + point[1]) * height * 0.5f + 0.5f));
  return pixelPos;
}

void Canvas::AddText(const vtkm::Matrix<vtkm::Float32, 4, 4>& transform,
                     vtkm::Float32 scale,
                     const vtkm::Vec<vtkm::Float32, 2>& anchor,
                     const vtkm::rendering::Color& color,
                     const std::string& text) const
{
  if (!Internals->FontTexture.IsValid())
  {
    if (!LoadFont())
    {
      return;
    }
  }

  vtkm::rendering::Canvas* self = const_cast<vtkm::rendering::Canvas*>(this);
  TextRenderer fontRenderer(self, Internals->Font, Internals->FontTexture);
  fontRenderer.RenderText(transform, scale, anchor, color, text);
}

void Canvas::AddText(const vtkm::Vec<vtkm::Float32, 2>& position,
                     vtkm::Float32 scale,
                     vtkm::Float32 angle,
                     vtkm::Float32 windowAspect,
                     const vtkm::Vec<vtkm::Float32, 2>& anchor,
                     const vtkm::rendering::Color& color,
                     const std::string& text) const
{
  vtkm::Matrix<vtkm::Float32, 4, 4> translationMatrix =
    Transform3DTranslate(position[0], position[1], 0.f);
  vtkm::Matrix<vtkm::Float32, 4, 4> scaleMatrix = Transform3DScale(1.0f / windowAspect, 1.0f, 1.0f);
  vtkm::Vec<vtkm::Float32, 3> rotationAxis(0.0f, 0.0f, 1.0f);
  vtkm::Matrix<vtkm::Float32, 4, 4> rotationMatrix = Transform3DRotate(angle, rotationAxis);
  vtkm::Matrix<vtkm::Float32, 4, 4> transform =
    vtkm::MatrixMultiply(translationMatrix, vtkm::MatrixMultiply(scaleMatrix, rotationMatrix));

  this->AddText(transform, scale, anchor, color, text);
}

void Canvas::AddText(vtkm::Float32 x,
                     vtkm::Float32 y,
                     vtkm::Float32 scale,
                     vtkm::Float32 angle,
                     vtkm::Float32 windowAspect,
                     vtkm::Float32 anchorX,
                     vtkm::Float32 anchorY,
                     const vtkm::rendering::Color& color,
                     const std::string& text) const
{
  this->AddText(vtkm::make_Vec(x, y),
                scale,
                angle,
                windowAspect,
                vtkm::make_Vec(anchorX, anchorY),
                color,
                text);
}

bool Canvas::LoadFont() const
{
  Internals->Font = BitmapFontFactory::CreateLiberation2Sans();
  const std::vector<unsigned char>& rawPNG = Internals->Font.GetRawImageData();
  std::vector<unsigned char> rgba;
  unsigned long textureWidth, textureHeight;
  int error = DecodePNG(rgba, textureWidth, textureHeight, &rawPNG[0], rawPNG.size());
  if (error != 0)
  {
    return false;
  }
  std::size_t numValues = textureWidth * textureHeight;
  std::vector<unsigned char> alpha(numValues);
  for (std::size_t i = 0; i < numValues; ++i)
  {
    alpha[i] = rgba[i * 4 + 3];
  }
  vtkm::cont::ArrayHandle<vtkm::UInt8> textureHandle = vtkm::cont::make_ArrayHandle(alpha);
  Internals->FontTexture =
    FontTextureType(vtkm::Id(textureWidth), vtkm::Id(textureHeight), textureHandle);
  Internals->FontTexture.SetFilterMode(TextureFilterMode::Linear);
  Internals->FontTexture.SetWrapMode(TextureWrapMode::Clamp);
  return true;
}

const vtkm::Matrix<vtkm::Float32, 4, 4>& Canvas::GetModelView() const
{
  return Internals->ModelView;
}

const vtkm::Matrix<vtkm::Float32, 4, 4>& Canvas::GetProjection() const
{
  return Internals->Projection;
}

void Canvas::SetViewToWorldSpace(const vtkm::rendering::Camera& camera, bool vtkmNotUsed(clip))
{
  Internals->ModelView = camera.CreateViewMatrix();
  Internals->Projection = camera.CreateProjectionMatrix(GetWidth(), GetHeight());
}

void Canvas::SetViewToScreenSpace(const vtkm::rendering::Camera& vtkmNotUsed(camera),
                                  bool vtkmNotUsed(clip))
{
  vtkm::MatrixIdentity(Internals->ModelView);
  vtkm::MatrixIdentity(Internals->Projection);
  Internals->Projection[2][2] = -1.0f;
}

void Canvas::SaveAs(const std::string& fileName) const
{
  this->RefreshColorBuffer();
  std::ofstream of(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
  vtkm::Id width = GetWidth();
  vtkm::Id height = GetHeight();
  of << "P6" << std::endl << width << " " << height << std::endl << 255 << std::endl;
  ColorBufferType::PortalConstControl colorPortal = GetColorBuffer().GetPortalConstControl();
  for (vtkm::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < width; xIndex++)
    {
      vtkm::Vec<vtkm::Float32, 4> tuple = colorPortal.Get(yIndex * width + xIndex);
      of << (unsigned char)(tuple[0] * 255);
      of << (unsigned char)(tuple[1] * 255);
      of << (unsigned char)(tuple[2] * 255);
    }
  }
  of.close();
}

vtkm::rendering::WorldAnnotator* Canvas::CreateWorldAnnotator() const
{
  return new vtkm::rendering::WorldAnnotator(this);
}
}
} // vtkm::rendering
