//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Quad_Extractor_h
#define vtk_m_rendering_raytracing_Quad_Extractor_h

#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class QuadExtractor
{
protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>> QuadIds;
  vtkm::cont::ArrayHandle<vtkm::Float32> Radii;

public:
  void ExtractCells(const vtkm::cont::DynamicCellSet& cells);

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 5>> GetQuadIds();

  vtkm::Id GetNumberOfQuads() const;

protected:
  void SetQuadIdsFromCells(const vtkm::cont::DynamicCellSet& cells);

}; // class ShapeIntersector
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Shape_Extractor_h
