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
#ifndef vtk_m_rendering_Scene_h
#define vtk_m_rendering_Scene_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Mapper.h>

namespace vtkm {
namespace rendering {

class Scene
{
public:
  VTKM_RENDERING_EXPORT
  Scene();

  VTKM_RENDERING_EXPORT
  void AddActor(const vtkm::rendering::Actor &actor);

  VTKM_RENDERING_EXPORT
  const vtkm::rendering::Actor &GetActor(vtkm::IdComponent index) const;

  VTKM_RENDERING_EXPORT
  vtkm::IdComponent GetNumberOfActors() const;

  VTKM_RENDERING_EXPORT
  void Render(vtkm::rendering::Mapper &mapper,
              vtkm::rendering::Canvas &canvas,
              const vtkm::rendering::Camera &camera) const;

  VTKM_RENDERING_EXPORT
  vtkm::Bounds GetSpatialBounds() const;

private:
  struct InternalsType;
  boost::shared_ptr<InternalsType> Internals;
};
}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Scene_h
