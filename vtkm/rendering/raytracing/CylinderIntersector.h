//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Cylinder_Intersector_h
#define vtk_m_rendering_raytracing_Cylinder_Intersector_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/rendering/raytracing/ShapeIntersector.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{
}
class CylinderIntersector : public ShapeIntersector
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Id3> CylIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Radii;

public:
  CylinderIntersector();
  virtual ~CylinderIntersector() override;


  void SetData(const vtkm::cont::CoordinateSystem& coords,
               vtkm::cont::ArrayHandle<vtkm::Id3> cylIds,
               vtkm::cont::ArrayHandle<vtkm::Float32> radii);

  void IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex = false) override;


  void IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex = false) override;

  template <typename Precision>
  void IntersectRaysImp(Ray<Precision>& rays, bool returnCellIndex);


  template <typename Precision>
  void IntersectionDataImp(Ray<Precision>& rays,
                           const vtkm::cont::Field scalarField,
                           const vtkm::Range& scalarRange);

  void IntersectionData(Ray<vtkm::Float32>& rays,
                        const vtkm::cont::Field scalarField,
                        const vtkm::Range& scalarRange) override;

  void IntersectionData(Ray<vtkm::Float64>& rays,
                        const vtkm::cont::Field scalarField,
                        const vtkm::Range& scalarRange) override;

  vtkm::Id GetNumberOfShapes() const override;
}; // class ShapeIntersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Shape_Intersector_h
