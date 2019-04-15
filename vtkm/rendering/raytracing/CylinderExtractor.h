//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Cylinder_Extractor_h
#define vtk_m_rendering_raytracing_Cylinder_Extractor_h

#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

/**
 * \brief CylinderExtractor creates a line segments from
 *        the edges of a cell set.
 *
 */
class CylinderExtractor
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Id3> CylIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Radii;

public:
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


  vtkm::cont::ArrayHandle<vtkm::Id3> GetCylIds();

  vtkm::cont::ArrayHandle<vtkm::Float32> GetRadii();
  vtkm::Id GetNumberOfCylinders() const;

protected:
  void SetUniformRadius(const vtkm::Float32 radius);
  void SetVaryingRadius(const vtkm::Float32 minRadius,
                        const vtkm::Float32 maxRadius,
                        const vtkm::cont::Field& field);

  //  void SetPointIdsFromCoords(const vtkm::cont::CoordinateSystem& coords);
  void SetCylinderIdsFromCells(const vtkm::cont::DynamicCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Shape_Extractor_h
