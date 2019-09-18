//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Shape_Intersector_h
#define vtk_m_rendering_raytracing_Shape_Intersector_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/rendering/raytracing/BoundingVolumeHierarchy.h>
#include <vtkm/rendering/raytracing/Ray.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT ShapeIntersector
{
protected:
  LinearBVH BVH;
  vtkm::cont::CoordinateSystem CoordsHandle;
  vtkm::Bounds ShapeBounds;
  void SetAABBs(AABBs& aabbs);

public:
  ShapeIntersector();
  virtual ~ShapeIntersector();

  //
  //  Intersect Rays finds the nearest intersection shape contained in the derived
  //  class in between min and max distances. HitIdx will be set to the local
  //  primitive id unless returnCellIndex is set to true. Cells are often
  //  decomposed into triangles and setting returnCellIndex to true will set
  //  HitIdx to the id of the cell.
  //
  virtual void IntersectRays(Ray<vtkm::Float32>& rays, bool returnCellIndex = false) = 0;


  virtual void IntersectRays(Ray<vtkm::Float64>& rays, bool returnCellIndex = false) = 0;

  //
  // Calling intersection data directly after IntersectRays popoulates
  // ray data: intersection point, surface normal, and interpolated scalar
  // value at the intersection location. Additionally, distance to intersection
  // becomes the new max distance.
  //
  virtual void IntersectionData(Ray<vtkm::Float32>& rays,
                                const vtkm::cont::Field scalarField,
                                const vtkm::Range& scalarRange) = 0;

  virtual void IntersectionData(Ray<vtkm::Float64>& rays,
                                const vtkm::cont::Field scalarField,
                                const vtkm::Range& scalarRange) = 0;


  template <typename Precision>
  void IntersectionPointImp(Ray<Precision>& rays);
  void IntersectionPoint(Ray<vtkm::Float32>& rays);
  void IntersectionPoint(Ray<vtkm::Float64>& rays);

  vtkm::Bounds GetShapeBounds() const;
  virtual vtkm::Id GetNumberOfShapes() const = 0;
}; // class ShapeIntersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Shape_Intersector_h
