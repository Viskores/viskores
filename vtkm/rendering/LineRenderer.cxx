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

#include <vtkm/rendering/LineRenderer.h>

#include <vtkm/Transform3D.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{

LineRenderer::LineRenderer(const vtkm::rendering::Canvas* canvas,
                           vtkm::Matrix<vtkm::Float32, 4, 4> transform)
  : Canvas(canvas)
  , Transform(transform)
{
}

void LineRenderer::RenderLine(const vtkm::Vec<vtkm::Float64, 2>& point0,
                              const vtkm::Vec<vtkm::Float64, 2>& point1,
                              vtkm::Float32 lineWidth,
                              const vtkm::rendering::Color& color)
{
  RenderLine(vtkm::make_Vec(point0[0], point0[1], 0.0),
             vtkm::make_Vec(point1[0], point1[1], 0.0),
             lineWidth,
             color);
}

void LineRenderer::RenderLine(const vtkm::Vec<vtkm::Float64, 3>& point0,
                              const vtkm::Vec<vtkm::Float64, 3>& point1,
                              vtkm::Float32 vtkmNotUsed(lineWidth),
                              const vtkm::rendering::Color& color)
{
  vtkm::Vec<vtkm::Float32, 3> p0 = TransformPoint(point0);
  vtkm::Vec<vtkm::Float32, 3> p1 = TransformPoint(point1);

  vtkm::Id x0 = static_cast<vtkm::Id>(vtkm::Round(p0[0]));
  vtkm::Id y0 = static_cast<vtkm::Id>(vtkm::Round(p0[1]));
  vtkm::Float32 z0 = static_cast<vtkm::Float32>(p0[2]);
  vtkm::Id x1 = static_cast<vtkm::Id>(vtkm::Round(p1[0]));
  vtkm::Id y1 = static_cast<vtkm::Id>(vtkm::Round(p1[1]));
  vtkm::Float32 z1 = static_cast<vtkm::Float32>(p1[2]);
  vtkm::Id dx = vtkm::Abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  vtkm::Id dy = -vtkm::Abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  vtkm::Id err = dx + dy, err2 = 0;
  auto colorPortal =
    vtkm::rendering::Canvas::ColorBufferType(Canvas->GetColorBuffer()).GetPortalControl();
  auto depthPortal =
    vtkm::rendering::Canvas::DepthBufferType(Canvas->GetDepthBuffer()).GetPortalControl();
  vtkm::Vec<vtkm::Float32, 4> colorC = color.Components;

  while (x0 >= 0 && x0 < Canvas->GetWidth() && y0 >= 0 && y0 < Canvas->GetHeight())
  {
    vtkm::Float32 t = (dx == 0) ? 1.0f : (static_cast<vtkm::Float32>(x0) - p0[0]) / (p1[0] - p0[0]);
    t = vtkm::Min(1.f, vtkm::Max(0.f, t));
    vtkm::Float32 z = vtkm::Lerp(z0, z1, t);
    vtkm::Id index = y0 * Canvas->GetWidth() + x0;
    vtkm::Vec<vtkm::Float32, 4> currentColor = colorPortal.Get(index);
    vtkm::Float32 currentZ = depthPortal.Get(index);
    bool blend = currentColor[3] < 1.f && z > currentZ;
    if (currentZ > z || blend)
    {
      vtkm::Vec<vtkm::Float32, 4> writeColor = colorC;
      vtkm::Float32 depth = z;

      if (blend)
      {
        // If there is any transparency, all alphas
        // have been pre-mulitplied
        vtkm::Float32 alpha = (1.f - currentColor[3]);
        writeColor[0] = currentColor[0] + colorC[0] * alpha;
        writeColor[1] = currentColor[1] + colorC[1] * alpha;
        writeColor[2] = currentColor[2] + colorC[2] * alpha;
        writeColor[3] = 1.f * alpha + currentColor[3]; // we are always drawing opaque lines
        // keep the current z. Line z interpolation is not accurate
        depth = currentZ;
      }

      depthPortal.Set(index, depth);
      colorPortal.Set(index, writeColor);
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

vtkm::Vec<vtkm::Float32, 3> LineRenderer::TransformPoint(
  const vtkm::Vec<vtkm::Float64, 3>& point) const
{
  vtkm::Vec<vtkm::Float32, 4> temp(static_cast<vtkm::Float32>(point[0]),
                                   static_cast<vtkm::Float32>(point[1]),
                                   static_cast<vtkm::Float32>(point[2]),
                                   1.0f);
  temp = vtkm::MatrixMultiply(Transform, temp);
  vtkm::Vec<vtkm::Float32, 3> p;
  for (vtkm::IdComponent i = 0; i < 3; ++i)
  {
    p[i] = static_cast<vtkm::Float32>(temp[i] / temp[3]);
  }
  p[0] = (p[0] * 0.5f + 0.5f) * static_cast<vtkm::Float32>(Canvas->GetWidth());
  p[1] = (p[1] * 0.5f + 0.5f) * static_cast<vtkm::Float32>(Canvas->GetHeight());
  p[2] = (p[2] * 0.5f + 0.5f) - 0.001f;
  return p;
}
}
} // namespace vtkm::rendering
