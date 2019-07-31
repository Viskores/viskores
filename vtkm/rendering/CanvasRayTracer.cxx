//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/CanvasRayTracer.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace rendering
{
namespace internal
{

class SurfaceConverter : public vtkm::worklet::WorkletMapField
{
  vtkm::Matrix<vtkm::Float32, 4, 4> ViewProjMat;

public:
  VTKM_CONT
  SurfaceConverter(const vtkm::Matrix<vtkm::Float32, 4, 4> viewProjMat)
    : ViewProjMat(viewProjMat)
  {
  }

  using ControlSignature =
    void(FieldIn, WholeArrayInOut, FieldIn, FieldIn, FieldIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);
  template <typename Precision,
            typename ColorPortalType,
            typename DepthBufferPortalType,
            typename ColorBufferPortalType>
  VTKM_EXEC void operator()(const vtkm::Id& pixelIndex,
                            ColorPortalType& colorBufferIn,
                            const Precision& inDepth,
                            const vtkm::Vec<Precision, 3>& origin,
                            const vtkm::Vec<Precision, 3>& dir,
                            DepthBufferPortalType& depthBuffer,
                            ColorBufferPortalType& colorBuffer,
                            const vtkm::Id& index) const
  {
    vtkm::Vec<Precision, 3> intersection = origin + inDepth * dir;
    vtkm::Vec4f_32 point;
    point[0] = static_cast<vtkm::Float32>(intersection[0]);
    point[1] = static_cast<vtkm::Float32>(intersection[1]);
    point[2] = static_cast<vtkm::Float32>(intersection[2]);
    point[3] = 1.f;

    vtkm::Vec4f_32 newpoint;
    newpoint = vtkm::MatrixMultiply(this->ViewProjMat, point);
    newpoint[0] = newpoint[0] / newpoint[3];
    newpoint[1] = newpoint[1] / newpoint[3];
    newpoint[2] = newpoint[2] / newpoint[3];

    vtkm::Float32 depth = newpoint[2];

    depth = 0.5f * (depth) + 0.5f;
    vtkm::Vec4f_32 color;
    color[0] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 0));
    color[1] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 1));
    color[2] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 2));
    color[3] = static_cast<vtkm::Float32>(colorBufferIn.Get(index * 4 + 3));
    // blend the mapped color with existing canvas color
    vtkm::Vec4f_32 inColor = colorBuffer.Get(pixelIndex);

    // if transparency exists, all alphas have been pre-multiplied
    vtkm::Float32 alpha = (1.f - color[3]);
    color[0] = color[0] + inColor[0] * alpha;
    color[1] = color[1] + inColor[1] * alpha;
    color[2] = color[2] + inColor[2] * alpha;
    color[3] = inColor[3] * alpha + color[3];

    // clamp
    for (vtkm::Int32 i = 0; i < 4; ++i)
    {
      color[i] = vtkm::Min(1.f, vtkm::Max(color[i], 0.f));
    }
    // The existing depth should already been feed into the ray mapper
    // so no color contribution will exist past the existing depth.

    depthBuffer.Set(pixelIndex, depth);
    colorBuffer.Set(pixelIndex, color);
  }
}; //class SurfaceConverter

template <typename Precision>
VTKM_CONT void WriteToCanvas(const vtkm::rendering::raytracing::Ray<Precision>& rays,
                             const vtkm::cont::ArrayHandle<Precision>& colors,
                             const vtkm::rendering::Camera& camera,
                             vtkm::rendering::CanvasRayTracer* canvas)
{
  vtkm::Matrix<vtkm::Float32, 4, 4> viewProjMat =
    vtkm::MatrixMultiply(camera.CreateProjectionMatrix(canvas->GetWidth(), canvas->GetHeight()),
                         camera.CreateViewMatrix());

  vtkm::worklet::DispatcherMapField<SurfaceConverter>(SurfaceConverter(viewProjMat))
    .Invoke(rays.PixelIdx,
            colors,
            rays.Distance,
            rays.Origin,
            rays.Dir,
            canvas->GetDepthBuffer(),
            canvas->GetColorBuffer());

  //Force the transfer so the vectors contain data from device
  canvas->GetColorBuffer().GetPortalControl().Get(0);
  canvas->GetDepthBuffer().GetPortalControl().Get(0);
}

} // namespace internal

CanvasRayTracer::CanvasRayTracer(vtkm::Id width, vtkm::Id height)
  : Canvas(width, height)
{
}

CanvasRayTracer::~CanvasRayTracer()
{
}

void CanvasRayTracer::WriteToCanvas(const vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays,
                                    const vtkm::cont::ArrayHandle<vtkm::Float32>& colors,
                                    const vtkm::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

void CanvasRayTracer::WriteToCanvas(const vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays,
                                    const vtkm::cont::ArrayHandle<vtkm::Float64>& colors,
                                    const vtkm::rendering::Camera& camera)
{
  internal::WriteToCanvas(rays, colors, camera, this);
}

vtkm::rendering::Canvas* CanvasRayTracer::NewCopy() const
{
  return new vtkm::rendering::CanvasRayTracer(*this);
}
}
}
