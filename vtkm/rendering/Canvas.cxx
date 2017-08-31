//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/Canvas.h>

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/TryExecute.h>
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
  }

  template <typename FrameBuffer, typename ColorMap>
  VTKM_EXEC void operator()(const vtkm::Id& index,
                            FrameBuffer& frameBuffer,
                            const ColorMap& colorMap) const
  {
    // local bar coord
    vtkm::Id x = index % BarWidth;
    vtkm::Id y = index / BarWidth;
    vtkm::Id sample = Horizontal ? x : y;
    vtkm::Vec<vtkm::Float32, 4> color = colorMap.Get(sample);

    // offset to global image coord
    x += BarBottomLeft[0];
    y += BarBottomLeft[1];

    vtkm::Id offset = y * ImageWidth + x;
    frameBuffer.Set(offset, color);
  }

  vtkm::Id ImageWidth;
  vtkm::Id ImageHeight;
  vtkm::Id2 BarBottomLeft;
  vtkm::Id BarWidth;
  bool Horizontal;
}; // struct DrawColorBar

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
};

} // namespace internal

Canvas::Canvas(vtkm::Id width, vtkm::Id height)
  : Width(0)
  , Height(0)
{
  this->ResizeBuffers(width, height);
}

Canvas::~Canvas()
{
}

vtkm::rendering::Canvas* Canvas::NewCopy() const
{
  return new vtkm::rendering::Canvas(*this);
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
  vtkm::cont::TryExecute(
    internal::ClearBuffersExecutor(this->GetColorBuffer(), this->GetDepthBuffer()));
}

void Canvas::Finish()
{
}

void Canvas::BlendBackground()
{
  vtkm::cont::TryExecute(internal::BlendBackgroundExecutor(this->GetColorBuffer(),
                                                           this->GetBackgroundColor().Components));
}

void Canvas::AddColorSwatch(const vtkm::Vec<vtkm::Float64, 2>& vtkmNotUsed(point0),
                            const vtkm::Vec<vtkm::Float64, 2>& vtkmNotUsed(point1),
                            const vtkm::Vec<vtkm::Float64, 2>& vtkmNotUsed(point2),
                            const vtkm::Vec<vtkm::Float64, 2>& vtkmNotUsed(point3),
                            const vtkm::rendering::Color& vtkmNotUsed(color)) const
{
  // Not implemented
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
                     vtkm::Float32 vtkmNotUsed(linewidth),
                     const vtkm::rendering::Color& color) const
{
  const vtkm::Float32 width = static_cast<vtkm::Float32>(this->Width);
  const vtkm::Float32 height = static_cast<vtkm::Float32>(this->Height);
  vtkm::Id x0 = static_cast<vtkm::Id>(vtkm::Round((point0[0] * 0.5f + 0.5f) * width));
  vtkm::Id y0 = static_cast<vtkm::Id>(vtkm::Round((point0[1] * 0.5f + 0.5f) * height));
  vtkm::Id x1 = static_cast<vtkm::Id>(vtkm::Round((point1[0] * 0.5f + 0.5f) * width));
  vtkm::Id y1 = static_cast<vtkm::Id>(vtkm::Round((point1[1] * 0.5f + 0.5f) * height));
  vtkm::Id dx = vtkm::Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  vtkm::Id dy = -vtkm::Abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  vtkm::Id err = dx + dy, err2 = 0;
  ColorBufferType::PortalControl colorPortal =
    ColorBufferType(this->ColorBuffer).GetPortalControl();

  while (true)
  {
    vtkm::Id index = y0 * this->Width + x0;
    colorPortal.Set(index, color.Components);
    if (x0 == x1 && y0 == y1)
    {
      break;
    }
    err2 = err * 2;
    if (err2 >= dy)
    {
      err += dy;
      x0 += sx;
    }
    if (err2 <= dx)
    {
      err += dx;
      y0 += sy;
    }
  }
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

void Canvas::AddText(const vtkm::Vec<vtkm::Float32, 2>& vtkmNotUsed(position),
                     vtkm::Float32 vtkmNotUsed(scale),
                     vtkm::Float32 vtkmNotUsed(angle),
                     vtkm::Float32 vtkmNotUsed(windowAspect),
                     const vtkm::Vec<vtkm::Float32, 2>& vtkmNotUsed(anchor),
                     const vtkm::rendering::Color& vtkmNotUsed(color),
                     const std::string& vtkmNotUsed(text)) const
{
  // Not implemented
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

void Canvas::SaveAs(const std::string& fileName) const
{
  this->RefreshColorBuffer();
  std::ofstream of(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
  of << "P6" << std::endl << this->Width << " " << this->Height << std::endl << 255 << std::endl;
  ColorBufferType::PortalConstControl colorPortal = this->ColorBuffer.GetPortalConstControl();
  for (vtkm::Id yIndex = this->Height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < this->Width; xIndex++)
    {
      vtkm::Vec<vtkm::Float32, 4> tuple = colorPortal.Get(yIndex * this->Width + xIndex);
      of << (unsigned char)(tuple[0] * 255);
      of << (unsigned char)(tuple[1] * 255);
      of << (unsigned char)(tuple[2] * 255);
    }
  }
  of.close();
}

vtkm::rendering::WorldAnnotator* Canvas::CreateWorldAnnotator() const
{
  return new vtkm::rendering::WorldAnnotator;
}
}
} // vtkm::rendering
