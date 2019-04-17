//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_CanvasRayTracer_h
#define vtk_m_rendering_CanvasRayTracer_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/raytracing/Ray.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT CanvasRayTracer : public Canvas
{
public:
  CanvasRayTracer(vtkm::Id width = 1024, vtkm::Id height = 1024);

  ~CanvasRayTracer();

  vtkm::rendering::Canvas* NewCopy() const override;

  void WriteToCanvas(const vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays,
                     const vtkm::cont::ArrayHandle<vtkm::Float32>& colors,
                     const vtkm::rendering::Camera& camera);

  void WriteToCanvas(const vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays,
                     const vtkm::cont::ArrayHandle<vtkm::Float64>& colors,
                     const vtkm::rendering::Camera& camera);
}; // class CanvasRayTracer
}
} // namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasRayTracer_h
