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

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vector>

namespace vtkm {
namespace rendering {

class Scene
{
public:
  VTKM_CONT_EXPORT
  void AddActor(const vtkm::rendering::Actor &actor)
  {
    this->Actors.push_back(actor);
  }

  VTKM_CONT_EXPORT
  const vtkm::rendering::Actor &GetActor(vtkm::IdComponent index) const
  {
    return this->Actors[static_cast<std::size_t>(index)];
  }

  VTKM_CONT_EXPORT
  vtkm::IdComponent GetNumberOfActors() const
  {
    return static_cast<vtkm::IdComponent>(this->Actors.size());
  }

  Scene() {}

  template<typename MapperType, typename CanvasType>
  VTKM_CONT_EXPORT
  void Render(MapperType &mapper,
              CanvasType &canvas,
              vtkm::rendering::Camera &camera) const
  {
    vtkm::Bounds bounds;

    mapper.StartScene();
    for (vtkm::IdComponent actorIndex = 0;
         actorIndex < this->GetNumberOfActors();
         actorIndex++)
    {
      const vtkm::rendering::Actor &actor = this->GetActor(actorIndex);
      actor.Render(mapper, canvas, camera);

      // accumulate all Actors' spatial bounds into the scene spatial bounds
      bounds.Include(actor.SpatialBounds);
    }
    mapper.EndScene();

    const_cast<Scene*>(this)->SpatialBounds = bounds;
  }

  const vtkm::Bounds &GetSpatialBounds() const
  {
    return this->SpatialBounds;
  }

private:
  std::vector<vtkm::rendering::Actor> Actors;
  vtkm::Bounds SpatialBounds;
};
}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Scene_h
