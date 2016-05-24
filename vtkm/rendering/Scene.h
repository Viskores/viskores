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
    this->SpatialBounds[0] = this->SpatialBounds[2] = this->SpatialBounds[4] = +FLT_MAX;
    this->SpatialBounds[1] = this->SpatialBounds[3] = this->SpatialBounds[5] = -FLT_MAX;

    sceneRenderer.StartScene();
    for (std::size_t i = 0; i < this->Plots.size(); i++)
    {
      this->Plots[i].Render(sceneRenderer, surface, view);

      // accumulate all Plots' spatial bounds into the scene spatial bounds
      this->SpatialBounds[0] = vtkm::Min(this->SpatialBounds[0], this->Plots[i].spatialBounds[0]);
      this->SpatialBounds[1] = vtkm::Max(this->SpatialBounds[1], this->Plots[i].spatialBounds[1]);
      this->SpatialBounds[2] = vtkm::Min(this->SpatialBounds[2], this->Plots[i].spatialBounds[2]);
      this->SpatialBounds[3] = vtkm::Max(this->SpatialBounds[3], this->Plots[i].spatialBounds[3]);
      this->SpatialBounds[4] = vtkm::Min(this->SpatialBounds[4], this->Plots[i].spatialBounds[4]);
      this->SpatialBounds[5] = vtkm::Max(this->SpatialBounds[5], this->Plots[i].spatialBounds[5]);
    }
    sceneRenderer.EndScene();
  }

  vtkm::Float64 *GetSpatialBounds()
  {
    return this->SpatialBounds;
  }

protected:
  vtkm::Float64 SpatialBounds[6];
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
