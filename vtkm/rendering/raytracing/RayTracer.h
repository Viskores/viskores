//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_RayTracer_h
#define vtk_m_rendering_raytracing_RayTracer_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Camera.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT RayTracer
{
protected:
  LinearBVH Bvh;
  Camera camera;
  vtkm::cont::ArrayHandleVirtualCoordinates CoordsHandle;
  vtkm::cont::Field ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> Indices;
  vtkm::cont::ArrayHandle<vtkm::Float32> Scalars;
  vtkm::Id NumberOfTriangles;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorMap;
  vtkm::Range ScalarRange;
  vtkm::Bounds DataBounds;
  template <typename Precision>
  struct RenderFunctor;

  template <typename Device, typename Precision>
  void RenderOnDevice(Ray<Precision>& rays, Device);

public:
  VTKM_CONT
  RayTracer();

  VTKM_CONT
  Camera& GetCamera();

  VTKM_CONT
  void SetData(const vtkm::cont::ArrayHandleVirtualCoordinates& coordsHandle,
               const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>>& indices,
               vtkm::cont::Field& scalarField,
               const vtkm::Id& numberOfTriangles,
               const vtkm::Range& scalarRange,
               const vtkm::Bounds& dataBounds);

  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap);

  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays);

}; //class RayTracer
}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracer_h
