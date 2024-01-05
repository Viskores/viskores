//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Mapper_h
#define vtk_m_rendering_Mapper_h

#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
namespace vtkm
{
namespace rendering
{

/// @brief Converts data into commands to a rendering system.
///
/// This is the base class for all mapper classes in VTK-m. Different concrete
/// derived classes can provide different representations and rendering techniques.
class VTKM_RENDERING_EXPORT Mapper
{
public:
  VTKM_CONT
  Mapper() {}

  virtual ~Mapper();

  virtual void RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                           const vtkm::cont::CoordinateSystem& coords,
                           const vtkm::cont::Field& scalarField,
                           const vtkm::cont::ColorTable& colorTable,
                           const vtkm::rendering::Camera& camera,
                           const vtkm::Range& scalarRange);

  void RenderCells(const vtkm::cont::UnknownCellSet& cellset,
                   const vtkm::cont::CoordinateSystem& coords,
                   const vtkm::cont::Field& scalarField,
                   const vtkm::cont::ColorTable& colorTable,
                   const vtkm::rendering::Camera& camera,
                   const vtkm::Range& scalarRange,
                   const vtkm::cont::Field& ghostField);

  virtual void RenderCellsPartitioned(const vtkm::cont::PartitionedDataSet partitionedData,
                                      const std::string fieldName,
                                      const vtkm::cont::ColorTable& colorTable,
                                      const vtkm::rendering::Camera& camera,
                                      const vtkm::Range& scalarRange);

  virtual void SetActiveColorTable(const vtkm::cont::ColorTable& ct);

  virtual void SetCanvas(vtkm::rendering::Canvas* canvas) = 0;
  virtual vtkm::rendering::Canvas* GetCanvas() const = 0;

  virtual vtkm::rendering::Mapper* NewCopy() const = 0;

  virtual void SetLogarithmX(bool l);
  virtual void SetLogarithmY(bool l);

protected:
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> ColorMap;
  bool LogarithmX = false;
  bool LogarithmY = false;

  // for the volume renderer sorting back to front gives better results for transarent colors, which is the default
  // but for the raytracer front to back is better.
  bool SortBackToFront = true;

  virtual void RenderCellsImpl(const vtkm::cont::UnknownCellSet& cellset,
                               const vtkm::cont::CoordinateSystem& coords,
                               const vtkm::cont::Field& scalarField,
                               const vtkm::cont::ColorTable& colorTable,
                               const vtkm::rendering::Camera& camera,
                               const vtkm::Range& scalarRange,
                               const vtkm::cont::Field& ghostField) = 0;
};
}
} //namespace vtkm::rendering
#endif //vtk_m_rendering_Mapper_h
