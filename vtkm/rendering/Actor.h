//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_Actor_h
#define vtk_m_rendering_Actor_h

#include <vtkm/Assert.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Mapper.h>
#include <vector>

namespace vtkm {
namespace rendering {

class Actor
{
public:
  //Actor(points, cells, field, colortable) {}
  VTKM_CONT_EXPORT
  Actor(const vtkm::cont::DynamicCellSet &cells,
        const vtkm::cont::CoordinateSystem &coordinates,
        const vtkm::cont::Field &scalarField,
        const vtkm::rendering::ColorTable &colorTable)
    : Cells(cells),
      Coordinates(coordinates),
      ScalarField(scalarField),
      ColorTable(colorTable)
  {
    VTKM_ASSERT(scalarField.GetData().GetNumberOfComponents() == 1);

    scalarField.GetRange(&this->ScalarRange,
                         VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    this->SpatialBounds =
        coordinates.GetBounds(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
  }

  template<typename MapperType, typename SurfaceType>
  VTKM_CONT_EXPORT
  void Render(MapperType &mapper,
              SurfaceType &surface,
              vtkm::rendering::Camera &camera)
  {
    mapper.SetRenderSurface(&surface);
    mapper.SetActiveColorTable(this->ColorTable);
    mapper.RenderCells(this->Cells,
                       this->Coordinates,
                       this->ScalarField,
                       this->ColorTable,
                       camera,
                       this->ScalarRange);
  }

  vtkm::cont::DynamicCellSet Cells;
  vtkm::cont::CoordinateSystem Coordinates;
  vtkm::cont::Field ScalarField;
  vtkm::rendering::ColorTable ColorTable;

  vtkm::Range ScalarRange;
  vtkm::Bounds SpatialBounds;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Actor_h
