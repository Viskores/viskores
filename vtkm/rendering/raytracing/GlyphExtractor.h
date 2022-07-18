//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Glyph_Extractor_h
#define vtk_m_rendering_raytracing_Glyph_Extractor_h

#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class GlyphExtractor
{
public:
  GlyphExtractor();

  //
  // Extract all nodes using a constant size
  //
  void ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords, const vtkm::Float32 size);

  //
  // Set size based on scalar field values. Each is interpolated from min to max
  //
  void ExtractCoordinates(const vtkm::cont::CoordinateSystem& coords,
                          const vtkm::cont::Field& field,
                          const vtkm::Float32 minSize,
                          const vtkm::Float32 maxSize);

  //
  // Extract all vertex shapes with constant size
  //
  void ExtractCells(const vtkm::cont::UnknownCellSet& cells, vtkm::Float32 size);

  //
  // Extract all vertex elements with size based on scalar values
  //
  void ExtractCells(const vtkm::cont::UnknownCellSet& cells,
                    const vtkm::cont::Field& field,
                    const vtkm::Float32 minSize,
                    const vtkm::Float32 maxSize);


  vtkm::cont::ArrayHandle<vtkm::Id> GetPointIds();
  vtkm::cont::ArrayHandle<vtkm::Float32> GetSizes();

  vtkm::Id GetNumberOfGlyphs() const;

protected:
  void SetUniformSize(const vtkm::Float32 size);
  void SetVaryingSize(const vtkm::Float32 minSize,
                      const vtkm::Float32 maxSize,
                      const vtkm::cont::Field& field);

  void SetPointIdsFromCoords(const vtkm::cont::CoordinateSystem& coords);
  void SetPointIdsFromCells(const vtkm::cont::UnknownCellSet& cells);

  vtkm::cont::ArrayHandle<vtkm::Id> PointIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Sizes;
}; // class GlyphExtractor
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Glyph_Extractor_h
