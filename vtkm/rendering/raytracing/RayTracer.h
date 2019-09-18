//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_RayTracer_h
#define vtk_m_rendering_raytracing_RayTracer_h

#include <memory>
#include <vector>

#include <vtkm/cont/DataSet.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/TriangleIntersector.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT RayTracer
{
protected:
  std::vector<std::shared_ptr<ShapeIntersector>> Intersectors;
  Camera camera;
  vtkm::cont::Field ScalarField;
  vtkm::cont::ArrayHandle<vtkm::Float32> Scalars;
  vtkm::Id NumberOfShapes;
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> ColorMap;
  vtkm::Range ScalarRange;
  bool Shade;

  template <typename Precision>
  void RenderOnDevice(Ray<Precision>& rays);

public:
  VTKM_CONT
  RayTracer();
  VTKM_CONT
  ~RayTracer();

  VTKM_CONT
  Camera& GetCamera();

  VTKM_CONT
  void AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector);

  VTKM_CONT
  void SetField(const vtkm::cont::Field& scalarField, const vtkm::Range& scalarRange);

  VTKM_CONT
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap);

  VTKM_CONT
  void SetShadingOn(bool on);

  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays);

  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays);

  VTKM_CONT
  vtkm::Id GetNumberOfShapes() const;

  VTKM_CONT
  void Clear();

}; //class RayTracer
}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracer_h
