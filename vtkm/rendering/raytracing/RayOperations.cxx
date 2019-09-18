//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/raytracing/RayOperations.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

void RayOperations::MapCanvasToRays(Ray<vtkm::Float32>& rays,
                                    const vtkm::rendering::Camera& camera,
                                    const vtkm::rendering::CanvasRayTracer& canvas)
{
  vtkm::Id width = canvas.GetWidth();
  vtkm::Id height = canvas.GetHeight();
  vtkm::Matrix<vtkm::Float32, 4, 4> projview =
    vtkm::MatrixMultiply(camera.CreateProjectionMatrix(width, height), camera.CreateViewMatrix());
  bool valid;
  vtkm::Matrix<vtkm::Float32, 4, 4> inverse = vtkm::MatrixInverse(projview, valid);
  (void)valid; // this can be a false negative for really tiny spatial domains.
  vtkm::worklet::DispatcherMapField<detail::RayMapCanvas>(
    detail::RayMapCanvas(inverse, width, height, camera.GetPosition()))
    .Invoke(rays.PixelIdx, rays.MaxDistance, canvas.GetDepthBuffer());
}
}
}
} // vtkm::rendering::raytacing
