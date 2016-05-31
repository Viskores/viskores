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

#include <vtkm/rendering/Plot.h>
#include <vtkm/rendering/View.h>
#include <vector>

namespace vtkm {
namespace rendering {

class Scene
{
public:
  std::vector<vtkm::rendering::Plot> Plots;
};

class Scene3D : public Scene
{
public:
  Scene3D() {}

  template<typename SceneRendererType, typename SurfaceType>
  VTKM_CONT_EXPORT
  void Render(SceneRendererType &sceneRenderer,
              SurfaceType &surface,
              vtkm::rendering::View &view)
  {
    vtkm::Bounds bounds;

    sceneRenderer.StartScene();
    for (std::size_t i = 0; i < this->Plots.size(); i++)
    {
      this->Plots[i].Render(sceneRenderer, surface, view);

      // accumulate all Plots' spatial bounds into the scene spatial bounds
      bounds.Include(this->Plots[i].SpatialBounds);
    }
    sceneRenderer.EndScene();

    this->SpatialBounds = bounds;
  }

  const vtkm::Bounds &GetSpatialBounds()
  {
    return this->SpatialBounds;
  }

protected:
  vtkm::Bounds SpatialBounds;
};

class Scene2D : public Scene
{
public:
  Scene2D() {}

  template<typename SceneRendererType, typename SurfaceType>
  VTKM_CONT_EXPORT
  void Render(SceneRendererType &sceneRenderer,
              SurfaceType &surface,
              vtkm::rendering::View &view)
  {
    for (std::size_t i = 0; i < this->Plots.size(); i++)
    {
      sceneRenderer.StartScene();
      this->Plots[i].Render(sceneRenderer, surface, view);
      sceneRenderer.EndScene();
    }
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Scene_h
