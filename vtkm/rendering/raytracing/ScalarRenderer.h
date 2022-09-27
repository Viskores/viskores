//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_ScalarRenderer_h
#define vtk_m_rendering_raytracing_ScalarRenderer_h

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

class VTKM_RENDERING_EXPORT ScalarRenderer
{
protected:
  std::shared_ptr<ShapeIntersector> Intersector;
  std::vector<vtkm::cont::Field> Fields;
  bool IntersectorValid;

  template <typename Precision>
  void RenderOnDevice(Ray<Precision>& rays,
                      Precision missScalar,
                      vtkm::rendering::raytracing::Camera& cam);

  template <typename Precision>
  void AddBuffer(Ray<Precision>& rays, Precision missScalar, const std::string name);

  template <typename Precision>
  void AddDepthBuffer(Ray<Precision>& rays);

public:
  VTKM_CONT
  ScalarRenderer();
  VTKM_CONT
  ~ScalarRenderer();

  VTKM_CONT
  void SetShapeIntersector(std::shared_ptr<ShapeIntersector> intersector);

  VTKM_CONT
  void AddField(const vtkm::cont::Field& scalarField);

  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float32>& rays,
              vtkm::Float32 missScalar,
              vtkm::rendering::raytracing::Camera& cam);

  VTKM_CONT
  void Render(vtkm::rendering::raytracing::Ray<vtkm::Float64>& rays,
              vtkm::Float64 missScalar,
              vtkm::rendering::raytracing::Camera& cam);

}; //class RayTracer
}
}
} // namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracer_h
