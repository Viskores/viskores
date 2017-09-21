//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/Scene.h>

#include <vector>

namespace vtkm
{
namespace rendering
{

struct Scene::InternalsType
{
  std::vector<vtkm::rendering::Actor> Actors;
};

Scene::Scene()
  : Internals(new InternalsType)
{
}

void Scene::AddActor(const vtkm::rendering::Actor& actor)
{
  this->Internals->Actors.push_back(actor);
}

const vtkm::rendering::Actor& Scene::GetActor(vtkm::IdComponent index) const
{
  return this->Internals->Actors[static_cast<std::size_t>(index)];
}

vtkm::IdComponent Scene::GetNumberOfActors() const
{
  return static_cast<vtkm::IdComponent>(this->Internals->Actors.size());
}

void Scene::Render(vtkm::rendering::Mapper& mapper,
                   vtkm::rendering::Canvas& canvas,
                   const vtkm::rendering::Camera& camera) const
{
  mapper.StartScene();
  for (vtkm::IdComponent actorIndex = 0; actorIndex < this->GetNumberOfActors(); actorIndex++)
  {
    const vtkm::rendering::Actor& actor = this->GetActor(actorIndex);
    actor.Render(mapper, canvas, camera);
  }
  mapper.EndScene();
}

vtkm::Bounds Scene::GetSpatialBounds() const
{
  vtkm::Bounds bounds;
  for (vtkm::IdComponent actorIndex = 0; actorIndex < this->GetNumberOfActors(); actorIndex++)
  {
    // accumulate all Actors' spatial bounds into the scene spatial bounds
    bounds.Include(this->GetActor(actorIndex).GetSpatialBounds());
  }

  return bounds;
}
}
} // namespace vtkm::rendering
