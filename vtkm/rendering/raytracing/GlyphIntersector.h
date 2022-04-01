//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Glyph_Intersector_h
#define vtk_m_rendering_raytracing_Glyph_Intersector_h

#include <vtkm/rendering/GlyphType.h>
#include <vtkm/rendering/raytracing/ShapeIntersector.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class GlyphIntersector : public ShapeIntersector
{
public:
  GlyphIntersector(vtkm::rendering::GlyphType glyphType);
  virtual ~GlyphIntersector() override;

  void SetGlyphType(vtkm::rendering::GlyphType glyphType);

  void SetData(const vtkm::cont::CoordinateSystem& coords,
               vtkm::cont::ArrayHandle<vtkm::Id> pointIds,
               vtkm::cont::ArrayHandle<vtkm::Float32> sizes);

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

protected:
  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Sizes;
  vtkm::rendering::GlyphType GlyphType;

}; // class GlyphIntersector

}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Glyph_Intersector_h
