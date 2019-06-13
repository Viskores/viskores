//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Sphere_Extractor_h
#define vtk_m_rendering_raytracing_Sphere_Extractor_h

#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class SphereExtractor
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Radii;

public:
  //
  // Extract all nodes using a constant radius
  //
  void ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords, const vtkm::Float32 radius);

  //
  // Set radius based on scalar field values. Each is interpolated from min to max
  //
  void ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                          const vtkm::cont::Field& field,
                          const vtkm::Float32 minRadius,
                          const vtkm::Float32 maxRadius);

  //
  // Extract all vertex shapes with constant radius
  //
  void ExtractCells(const vtkm::cont::DynamicCellSet& cells, vtkm::Float32 radius);

  //
  // Extract all vertex elements with radius based on scalar values
  //
  void ExtractCells(const vtkm::cont::DynamicCellSet& cells,
                    const vtkm::cont::Field& field,
                    const vtkm::Float32 minRadius,
                    const vtkm::Float32 maxRadius);


  vtkm::cont::ArrayHandle<vtkm::Id> GetPointIds();
  vtkm::cont::ArrayHandle<vtkm::Float32> GetRadii();
  vtkm::Id GetNumberOfSpheres() const;

protected:
  void SetUniformRadius(const vtkm::Float32 radius);
  void SetVaryingRadius(const vtkm::Float32 minRadius,
                        const vtkm::Float32 maxRadius,
                        const vtkm::cont::Field& field);

  void SetPointIdsFromCoords(const vtkm::cont::CoordinateSystem& coords);
  void SetPointIdsFromCells(const vtkm::cont::DynamicCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Shape_Extractor_h
