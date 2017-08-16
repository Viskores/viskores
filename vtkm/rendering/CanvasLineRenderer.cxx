//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/CanvasLineRenderer.h>

#include <fstream>

#include <vtkm/Assert.h>
#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace
{

class ClearBuffers : public vtkm::worklet::WorkletMapField
{
  vtkm::rendering::Color ClearColor;

public:
  VTKM_CONT
  ClearBuffers(const vtkm::rendering::Color& clearColor)
    : ClearColor(clearColor)
  {
  }

  typedef void ControlSignature(FieldOut<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);
  VTKM_EXEC
  void operator()(vtkm::Vec<vtkm::Float32, 4>& color, vtkm::Float32& depth) const
  {
    color = this->ClearColor.Components;
    // Set the depth buffer value to value slightly greater than 1.0f,
    // marking it as invalid and ensuring future depth checks valid range
    // of [0.0f, 1.0f] succeed.
    depth = 1.001f;
  }
}; //class ClearBuffers

struct ClearBuffersInvokeFunctor
{
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;
  typedef vtkm::rendering::Canvas::DepthBufferType DepthBufferType;

  ClearBuffers Worklet;
  ColorBufferType ColorBuffer;
  DepthBufferType DepthBuffer;

  VTKM_CONT
  ClearBuffersInvokeFunctor(const vtkm::rendering::Color& backgroundColor,
                            const ColorBufferType& colorBuffer,
                            const DepthBufferType& depthBuffer)
    : Worklet(backgroundColor)
    , ColorBuffer(colorBuffer)
    , DepthBuffer(depthBuffer)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::worklet::DispatcherMapField<ClearBuffers, Device> dispatcher(this->Worklet);
    dispatcher.Invoke(this->ColorBuffer, this->DepthBuffer);
    return true;
  }
};

vtkm::Float32 IPart(vtkm::Float32 x)
{
  return vtkm::Floor(x);
}

vtkm::Float32 FPart(vtkm::Float32 x)
{
  return x - vtkm::Floor(x);
}

vtkm::Float32 RFPart(vtkm::Float32 x)
{
  return 1.0f - FPart(x);
}

class LinePlotter : public vtkm::worklet::WorkletMapField
{
public:
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;

  VTKM_CONT
  LinePlotter(vtkm::Float32 x1,
              vtkm::Float32 y1,
              vtkm::Id width,
              vtkm::Id height,
              vtkm::Float32 gradient,
              const vtkm::rendering::Color& color,
              bool transposed)
    : X1(x1)
    , Y1(y1)
    , Width(width)
    , Height(height)
    , Gradient(gradient)
    , Color(color)
    , Transposed(transposed)
  {
  }

  typedef void ControlSignature(FieldIn<>, ExecObject);
  typedef void ExecutionSignature(_1, _2);

  VTKM_EXEC
  void operator()(const vtkm::Id x,
                  vtkm::exec::ExecutionWholeArray<ColorBufferType::ValueType>& colorBuffer) const
  {
    vtkm::Float32 y = Y1 + (static_cast<vtkm::Float32>(x) - X1) * Gradient;
    vtkm::Id yInt = static_cast<vtkm::Id>(y);
    if (Transposed)
    {
      BlendPixel(yInt, x, colorBuffer, RFPart(y));
      BlendPixel(yInt + 1, x, colorBuffer, FPart(y));
    }
    else
    {
      BlendPixel(x, yInt, colorBuffer, RFPart(y));
      BlendPixel(x, yInt + 1, colorBuffer, FPart(y));
    }
  }

private:
  VTKM_EXEC
  inline vtkm::Id GetBufferIndex(vtkm::Id x, vtkm::Id y) const { return y * Width + x; }

  VTKM_EXEC
  inline void BlendPixel(vtkm::Id x,
                         vtkm::Id y,
                         vtkm::exec::ExecutionWholeArray<ColorBufferType::ValueType>& colorBuffer,
                         vtkm::Float32 intensity) const
  {
    if (y < 0 || y >= Height)
    {
      return;
    }
    vtkm::Id index = this->GetBufferIndex(x, y);
    vtkm::Vec<vtkm::Float32, 4> dstColor(Color.Components);
    vtkm::Vec<vtkm::Float32, 4> srcColor(colorBuffer.Get(index));
    vtkm::Vec<vtkm::Float32, 4> blendedColor;
    blendedColor[0] = dstColor[0] * intensity + srcColor[0] * (1 - intensity);
    blendedColor[1] = dstColor[1] * intensity + srcColor[1] * (1 - intensity);
    blendedColor[2] = dstColor[2] * intensity + srcColor[2] * (1 - intensity);
    blendedColor[3] = 1.0f;
    colorBuffer.Set(index, blendedColor);
  }

  vtkm::Float32 X1, Y1;
  vtkm::Id Width, Height;
  vtkm::Float32 Gradient;
  vtkm::rendering::Color Color;
  bool Transposed;
}; //class LinePlotter

struct LinePlotterInvokeFunctor
{
  typedef vtkm::rendering::Canvas::ColorBufferType ColorBufferType;
  typedef vtkm::rendering::Canvas::DepthBufferType DepthBufferType;

  vtkm::Float32 X1, Y1;
  vtkm::Id Width, Height;
  vtkm::Float32 Gradient;
  vtkm::rendering::Color Color;
  bool Transposed;
  LinePlotter Worklet;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> Arr;
  vtkm::exec::ExecutionWholeArray<ColorBufferType::ValueType> Buffer;

  VTKM_CONT
  LinePlotterInvokeFunctor(vtkm::Float32 x1,
                           vtkm::Float32 y1,
                           vtkm::Id width,
                           vtkm::Id height,
                           vtkm::Float32 gradient,
                           const vtkm::rendering::Color& color,
                           bool transposed,
                           vtkm::cont::ArrayHandleCounting<vtkm::Id> arr,
                           ColorBufferType buffer)
    : Worklet(x1, y1, width, height, gradient, color, transposed)
    , Arr(arr)
    , Buffer(buffer)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::worklet::DispatcherMapField<LinePlotter, Device> dispatcher(this->Worklet);
    dispatcher.Invoke(this->Arr, this->Buffer);
    return true;
  }
};
} // namespace

CanvasLineRenderer::CanvasLineRenderer(vtkm::Id width, vtkm::Id height)
  : Canvas(width, height)
{
}

CanvasLineRenderer::~CanvasLineRenderer()
{
}

void CanvasLineRenderer::Initialize()
{
  // Nothing to initialize
}

void CanvasLineRenderer::Activate()
{
  // Nothing to activate
}

void CanvasLineRenderer::Finish()
{
  // Nothing to finish
}

void CanvasLineRenderer::Clear()
{
  vtkm::cont::TryExecute(ClearBuffersInvokeFunctor(
    this->GetBackgroundColor(), this->GetColorBuffer(), this->GetDepthBuffer()));
}

vtkm::rendering::Canvas* CanvasLineRenderer::NewCopy() const
{
  return new vtkm::rendering::CanvasLineRenderer(*this);
}


void CanvasLineRenderer::AddLine(const vtkm::Vec<vtkm::Float64, 2>& start,
                                 const vtkm::Vec<vtkm::Float64, 2>& end,
                                 vtkm::Float32 vtkmNotUsed(lineWidth),
                                 const vtkm::rendering::Color& color) const
{
  // Draw's a line from start to end using the specified color.
  // lineWidth is ignored for now.

  /*
  VTKM_ASSERT(start[0] >= 0.0f && start[0] < this->GetWidth()
              && start[1] >= 0.0f && start[1] < this->GetHeight()
              && end[0] >= 0.0f && end[0] < this->GetWidth()
              && end[1] >= 0.0f && end[1] < this->GetHeight());
  */
  vtkm::Float32 x1 = static_cast<vtkm::Float32>(start[0]);
  vtkm::Float32 y1 = static_cast<vtkm::Float32>(start[1]);
  vtkm::Float32 x2 = static_cast<vtkm::Float32>(end[0]);
  vtkm::Float32 y2 = static_cast<vtkm::Float32>(end[1]);

  // If the line is steep, i.e., the height is greater than the width, then
  // transpose the co-ordinates to prevent "holes" in the line. This ensures
  // that we pick the co-ordinate which grows at a lesser rate than the other.
  bool transposed = std::fabs(y2 - y1) > std::fabs(x2 - x1);
  if (transposed)
  {
    std::swap(x1, y1);
    std::swap(x2, y2);
    transposed = true;
  }
  // Ensure we are always going from left to right
  if (x1 > x2)
  {
    std::swap(x1, x2);
    std::swap(y1, y2);
  }

  vtkm::Float32 dx = x2 - x1;
  vtkm::Float32 dy = y2 - y1;
  vtkm::Float32 gradient = (dx == 0.0) ? 1.0f : (dy / dx);

  vtkm::Float32 xEnd = vtkm::Round(x1);
  vtkm::Float32 yEnd = y1 + gradient * (xEnd - x1);
  vtkm::Float32 xGap = RFPart(x1 + 0.5f);
  vtkm::Float32 xPxl1 = xEnd, yPxl1 = IPart(yEnd);

  if (transposed)
  {
    BlendPixel(yPxl1, xPxl1, color, RFPart(yEnd) * xGap);
    BlendPixel(yPxl1 + 1, xPxl1, color, FPart(yEnd) * xGap);
  }
  else
  {
    BlendPixel(xPxl1, yPxl1, color, RFPart(yEnd) * xGap);
    BlendPixel(xPxl1, yPxl1 + 1, color, FPart(yEnd) * xGap);
  }

  xEnd = vtkm::Round(x2);
  yEnd = y2 + gradient * (xEnd - x2);
  xGap = FPart(x2 + 0.5f);
  vtkm::Float32 xPxl2 = xEnd, yPxl2 = IPart(yEnd);

  if (transposed)
  {
    BlendPixel(yPxl2, xPxl2, color, RFPart(yEnd) * xGap);
    BlendPixel(yPxl2 + 1, xPxl2, color, FPart(yEnd) * xGap);
  }
  else
  {
    BlendPixel(xPxl2, yPxl2, color, RFPart(yEnd) * xGap);
    BlendPixel(xPxl2, yPxl2 + 1, color, FPart(yEnd) * xGap);
  }

  vtkm::cont::ArrayHandleCounting<vtkm::Id> xCoords(static_cast<vtkm::Id>(xPxl1 + 1),
                                                    static_cast<vtkm::Id>(1),
                                                    static_cast<vtkm::Id>(xPxl2 - xPxl1 - 1));
  ColorBufferType colorBuffer(this->GetColorBuffer());
  vtkm::cont::TryExecute(LinePlotterInvokeFunctor(x1,
                                                  y1,
                                                  this->GetWidth(),
                                                  this->GetHeight(),
                                                  gradient,
                                                  color,
                                                  transposed,
                                                  xCoords,
                                                  colorBuffer));
}

void CanvasLineRenderer::BlendPixel(vtkm::Float32 x,
                                    vtkm::Float32 y,
                                    const vtkm::rendering::Color& color,
                                    vtkm::Float32 intensity) const
{
  vtkm::Id xi = static_cast<vtkm::Id>(x), yi = static_cast<vtkm::Id>(y);
  if (!(xi >= 0 && xi < this->GetWidth() && yi >= 0 && yi < this->GetHeight()))
    return;
  vtkm::Id index = this->GetBufferIndex(static_cast<vtkm::Id>(x), static_cast<vtkm::Id>(y));
  ColorBufferType buffer(this->GetColorBuffer());
  vtkm::Vec<vtkm::Float32, 4> dstColor(color.Components);
  vtkm::Vec<vtkm::Float32, 4> srcColor(buffer.GetPortalConstControl().Get(index));
  vtkm::Vec<vtkm::Float32, 4> blendedColor;
  blendedColor[0] = dstColor[0] * intensity + srcColor[0] * (1 - intensity);
  blendedColor[1] = dstColor[1] * intensity + srcColor[1] * (1 - intensity);
  blendedColor[2] = dstColor[2] * intensity + srcColor[2] * (1 - intensity);
  blendedColor[3] = 1.0f;
  buffer.GetPortalControl().Set(index, blendedColor);
}

void CanvasLineRenderer::AddColorBar(const vtkm::Bounds&,
                                     const vtkm::rendering::ColorTable&,
                                     bool) const
{
  //TODO: Implement
}

void CanvasLineRenderer::AddText(const vtkm::Vec<vtkm::Float32, 2>&,
                                 vtkm::Float32,
                                 vtkm::Float32,
                                 vtkm::Float32,
                                 const vtkm::Vec<vtkm::Float32, 2>&,
                                 const vtkm::rendering::Color&,
                                 const std::string&) const
{
  //TODO: Implement
}

void CanvasLineRenderer::AddColorSwatch(const vtkm::Vec<vtkm::Float64, 2>& point0,
                                        const vtkm::Vec<vtkm::Float64, 2>& point1,
                                        const vtkm::Vec<vtkm::Float64, 2>& point2,
                                        const vtkm::Vec<vtkm::Float64, 2>& point3,
                                        const vtkm::rendering::Color& color) const
{
  //TODO: Implement
}


vtkm::Id CanvasLineRenderer::GetBufferIndex(vtkm::Id x, vtkm::Id y) const
{
  return y * this->GetWidth() + x;
}

} // namespace rendering
} // namespace vtkm
