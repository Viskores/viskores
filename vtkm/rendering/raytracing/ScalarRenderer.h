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
  std::vector<std::shared_ptr<ShapeIntersector>> Intersectors;
  std::vector<vtkm::cont::Field> Fields;
  Camera camera;
  vtkm::Id NumberOfShapes;

  template <typename Precision>
  void RenderOnDevice(Ray<Precision>& rays);

public:
  VTKM_CONT
  ScalarRenderer();
  VTKM_CONT
  ~ScalarRenderer();

  VTKM_CONT
  Camera& GetCamera();

  VTKM_CONT
  void AddShapeIntersector(std::shared_ptr<ShapeIntersector> intersector);

  VTKM_CONT
  void AddField(const vtkm::cont::Field& scalarField);

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
