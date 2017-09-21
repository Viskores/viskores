//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#include <vtkm/rendering/raytracing/RayOperations.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <typename T>
struct MapCanvasFunctor
{
  const vtkm::Matrix<vtkm::Float32, 4, 4> Inverse;
  const vtkm::Id Width;
  const vtkm::Id Height;
  const vtkm::cont::ArrayHandle<vtkm::Float32>& DepthBuffer;
  Ray<T>& Rays;
  vtkm::Vec<vtkm::Float32, 3> Origin;

  MapCanvasFunctor(Ray<T>& rays,
                   const vtkm::Matrix<vtkm::Float32, 4, 4> inverse,
                   const vtkm::Id width,
                   const vtkm::Id height,
                   const vtkm::cont::ArrayHandle<vtkm::Float32>& depthBuffer,
                   const vtkm::Vec<vtkm::Float32, 3>& origin)
    : Inverse(inverse)
    , Width(width)
    , Height(height)
    , DepthBuffer(depthBuffer)
    , Rays(rays)
    , Origin(origin)
  {
  }

  template <typename Device>
  bool operator()(Device)
  {
    vtkm::worklet::DispatcherMapField<detail::RayMapCanvas, Device>(
      detail::RayMapCanvas(Inverse, Width, Height, Origin))
      .Invoke(Rays.PixelIdx, Rays.MaxDistance, DepthBuffer);
    return true;
  }
};

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
  if (!valid)
    throw vtkm::cont::ErrorBadValue("Inverse Invalid");

  MapCanvasFunctor<vtkm::Float32> functor(
    rays, inverse, width, height, canvas.GetDepthBuffer(), camera.GetPosition());
  vtkm::cont::TryExecute(functor);
}
}
}
} // vtkm::rendering::raytacing
