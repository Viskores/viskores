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

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Mapper.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/shared_ptr.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace rendering {

class Actor
{
public:
  VTKM_RENDERING_EXPORT
  Actor(const vtkm::cont::DynamicCellSet &cells,
        const vtkm::cont::CoordinateSystem &coordinates,
        const vtkm::cont::Field &scalarField,
        const vtkm::rendering::ColorTable &colorTable =
          vtkm::rendering::ColorTable("default"));

  VTKM_RENDERING_EXPORT
  void Render(vtkm::rendering::Mapper &mapper,
              vtkm::rendering::Canvas &canvas,
              const vtkm::rendering::Camera &camera) const;

  VTKM_RENDERING_EXPORT
  const vtkm::cont::DynamicCellSet &GetCells() const;

  VTKM_RENDERING_EXPORT
  const vtkm::cont::CoordinateSystem &GetCoordiantes() const;

  VTKM_RENDERING_EXPORT
  const vtkm::cont::Field &GetScalarField() const;

  VTKM_RENDERING_EXPORT
  const vtkm::rendering::ColorTable &GetColorTable() const;

  VTKM_RENDERING_EXPORT
  const vtkm::Range &GetScalarRange() const;

  VTKM_RENDERING_EXPORT
  const vtkm::Bounds &GetSpatialBounds() const;

private:
  struct InternalsType;
  boost::shared_ptr<InternalsType> Internals;

  struct RangeFunctor;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Actor_h
