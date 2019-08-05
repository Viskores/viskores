//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_TriagnleIntersector_h
#define vtk_m_rendering_raytracing_TriagnleIntersector_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/ShapeIntersector.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT TriangleIntersector : public ShapeIntersector
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Id4> Triangles;
  bool UseWaterTight;

public:
  TriangleIntersector();

  void SetUseWaterTight(bool useIt);

  void SetData(const vtkm::cont::CoordinateSystem& coords,
               vtkm::cont::ArrayHandle<vtkm::Id4> triangles);

  vtkm::cont::ArrayHandle<vtkm::Id4> GetTriangles();
  vtkm::Id GetNumberOfShapes() const override;


  VTKM_CONT void IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex = false) override;
  VTKM_CONT void IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex = false) override;


  VTKM_CONT void IntersectionData(Ray<vtkm::Float32>& rays,
                                  const vtkm::cont::Field scalarField,
                                  const vtkm::Range& scalarRange) override;

  VTKM_CONT void IntersectionData(Ray<vtkm::Float64>& rays,
                                  const vtkm::cont::Field scalarField,
                                  const vtkm::Range& scalarRange) override;

  template <typename Precision>
  VTKM_CONT void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);

  template <typename Precision>
  VTKM_CONT void IntersectionDataImp(Ray<Precision>& rays,
                                     const vtkm::cont::Field scalarField,
                                     const vtkm::Range& scalarRange);

}; // class intersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_TriagnleIntersector_h
