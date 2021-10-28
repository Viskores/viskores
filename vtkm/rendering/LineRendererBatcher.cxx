//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/LineRendererBatcher.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace
{
using ColorsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
using PointsArrayHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f_32>;

struct RenderLine : public vtkm::worklet::WorkletMapField
{
  using ColorBufferType = vtkm::rendering::Canvas::ColorBufferType;
  using DepthBufferType = vtkm::rendering::Canvas::DepthBufferType;

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, WholeArrayInOut, WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  VTKM_CONT
  RenderLine() {}

  VTKM_CONT
  RenderLine(vtkm::Id width, vtkm::Id height)
    : Width(width)
    , Height(height)
  {
  }

  template <typename ColorBufferPortal, typename DepthBufferPortal>
  VTKM_EXEC void operator()(const vtkm::Vec3f_32& start,
                            const vtkm::Vec3f_32& end,
                            const vtkm::Vec4f_32& color,
                            ColorBufferPortal& colorBuffer,
                            DepthBufferPortal& depthBuffer) const
  {
    vtkm::Id x0 = static_cast<vtkm::Id>(vtkm::Round(start[0]));
    vtkm::Id y0 = static_cast<vtkm::Id>(vtkm::Round(start[1]));
    vtkm::Float32 z0 = static_cast<vtkm::Float32>(start[2]);
    vtkm::Id x1 = static_cast<vtkm::Id>(vtkm::Round(end[0]));
    vtkm::Id y1 = static_cast<vtkm::Id>(vtkm::Round(end[1]));
    vtkm::Float32 z1 = static_cast<vtkm::Float32>(end[2]);
    vtkm::Id dx = vtkm::Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    vtkm::Id dy = -vtkm::Abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    vtkm::Id err = dx + dy, err2 = 0;

    const vtkm::Id xStart = x0;
    const vtkm::Id yStart = y0;
    const vtkm::Float32 pdist = vtkm::Sqrt(vtkm::Float32(dx * dx) + vtkm::Float32(dy * dy));

    while (x0 >= 0 && x0 < this->Width && y0 >= 0 && y0 < this->Height)
    {
      vtkm::Float32 deltaX = static_cast<vtkm::Float32>(x0 - xStart);
      vtkm::Float32 deltaY = static_cast<vtkm::Float32>(y0 - yStart);
      // Depth is wrong, but its far less wrong that it used to be.
      // These depth values are in screen space, which have been
      // potentially tranformed by a perspective correction.
      // To interpolated the depth correctly, there must be a perspective correction.
      // I haven't looked, but the wireframmer probably suffers from this too.
      // Additionally, this should not happen on the CPU. Annotations take
      // far longer than the the geometry.
      vtkm::Float32 t = pdist == 0.f ? 1.0f : vtkm::Sqrt(deltaX * deltaX + deltaY * deltaY) / pdist;
      t = vtkm::Min(1.f, vtkm::Max(0.f, t));
      vtkm::Float32 z = vtkm::Lerp(z0, z1, t);

      vtkm::Id index = y0 * this->Width + x0;
      vtkm::Vec4f_32 currentColor = colorBuffer.Get(index);
      vtkm::Float32 currentZ = depthBuffer.Get(index);
      bool blend = currentColor[3] < 1.f && z > currentZ;
      if (currentZ > z || blend)
      {
        vtkm::Vec4f_32 writeColor = color;
        vtkm::Float32 depth = z;

        if (blend)
        {
          // If there is any transparency, all alphas
          // have been pre-mulitplied
          vtkm::Float32 alpha = (1.f - currentColor[3]);
          writeColor[0] = currentColor[0] + color[0] * alpha;
          writeColor[1] = currentColor[1] + color[1] * alpha;
          writeColor[2] = currentColor[2] + color[2] * alpha;
          writeColor[3] = 1.f * alpha + currentColor[3]; // we are always drawing opaque lines
          // keep the current z. Line z interpolation is not accurate
          // Matt: this is correct. Interpolation is wrong
          depth = currentZ;
        }

        depthBuffer.Set(index, depth);
        colorBuffer.Set(index, writeColor);
      }

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

  vtkm::Id Width;
  vtkm::Id Height;
}; // struct RenderLine
} // namespace

LineRendererBatcher::LineRendererBatcher() {}

void LineRendererBatcher::BatchLine(const vtkm::Vec3f_64& start,
                                    const vtkm::Vec3f_64& end,
                                    const vtkm::rendering::Color& color)
{
  vtkm::Vec3f_32 start32(static_cast<vtkm::Float32>(start[0]),
                         static_cast<vtkm::Float32>(start[1]),
                         static_cast<vtkm::Float32>(start[2]));
  vtkm::Vec3f_32 end32(static_cast<vtkm::Float32>(end[0]),
                       static_cast<vtkm::Float32>(end[1]),
                       static_cast<vtkm::Float32>(end[2]));
  this->BatchLine(start32, end32, color);
}

void LineRendererBatcher::BatchLine(const vtkm::Vec3f_32& start,
                                    const vtkm::Vec3f_32& end,
                                    const vtkm::rendering::Color& color)
{
  this->Starts.push_back(start);
  this->Ends.push_back(end);
  this->Colors.push_back(color.Components);
}

void LineRendererBatcher::Render(const vtkm::rendering::Canvas* canvas) const
{
  PointsArrayHandle starts = vtkm::cont::make_ArrayHandle(this->Starts, vtkm::CopyFlag::Off);
  PointsArrayHandle ends = vtkm::cont::make_ArrayHandle(this->Ends, vtkm::CopyFlag::Off);
  ColorsArrayHandle colors = vtkm::cont::make_ArrayHandle(this->Colors, vtkm::CopyFlag::Off);

  vtkm::cont::Invoker invoker;
  invoker(RenderLine(canvas->GetWidth(), canvas->GetHeight()),
          starts,
          ends,
          colors,
          canvas->GetColorBuffer(),
          canvas->GetDepthBuffer());
}
}
} // namespace vtkm::rendering
