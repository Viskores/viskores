//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_Mapper_h
#define vtk_m_rendering_Mapper_h

#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT Mapper
{
public:
  VTKM_CONT
  Mapper() {}

  virtual ~Mapper();

  virtual void RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                           const vtkm::cont::CoordinateSystem& coords,
                           const vtkm::cont::Field& scalarField,
                           const vtkm::cont::ColorTable& colorTable,
                           const vtkm::rendering::Camera& camera,
                           const vtkm::Range& scalarRange) = 0;

  virtual void SetActiveColorTable(const vtkm::cont::ColorTable& ct);

  virtual void StartScene() = 0;
  virtual void EndScene() = 0;
  virtual void SetCanvas(vtkm::rendering::Canvas* canvas) = 0;
  virtual vtkm::rendering::Canvas* GetCanvas() const = 0;

  virtual vtkm::rendering::Mapper* NewCopy() const = 0;

  virtual void SetLogarithmX(bool l);
  virtual void SetLogarithmY(bool l);

protected:
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorMap;
  bool LogarithmX = false;
  bool LogarithmY = false;
};
}
} //namespace vtkm::rendering
#endif //vtk_m_rendering_Mapper_h
