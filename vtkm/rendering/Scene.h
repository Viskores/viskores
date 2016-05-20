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
    std::vector<vtkm::rendering::Plot> plots;
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
        spatialBounds[0] = spatialBounds[2] = spatialBounds[4] = +FLT_MAX;
        spatialBounds[1] = spatialBounds[3] = spatialBounds[5] = -FLT_MAX;

        sceneRenderer.StartScene();
        for (std::size_t i = 0; i < plots.size(); i++)
        {
            plots[i].Render(sceneRenderer, surface, view);

            // accumulate all plots' spatial bounds into the scene spatial bounds
            spatialBounds[0] = vtkm::Min(spatialBounds[0], plots[i].spatialBounds[0]);
            spatialBounds[1] = vtkm::Max(spatialBounds[1], plots[i].spatialBounds[1]);
            spatialBounds[2] = vtkm::Min(spatialBounds[2], plots[i].spatialBounds[2]);
            spatialBounds[3] = vtkm::Max(spatialBounds[3], plots[i].spatialBounds[3]);
            spatialBounds[4] = vtkm::Min(spatialBounds[4], plots[i].spatialBounds[4]);
            spatialBounds[5] = vtkm::Max(spatialBounds[5], plots[i].spatialBounds[5]);
        }
        sceneRenderer.EndScene();
    }

    vtkm::Float64 *GetSpatialBounds()
    {
        return spatialBounds;
    }

protected:
    vtkm::Float64 spatialBounds[6];
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
        for (std::size_t i = 0; i < plots.size(); i++)
        {
            sceneRenderer.StartScene();
            plots[i].Render(sceneRenderer, surface, view);
            sceneRenderer.EndScene();
        }
    }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Scene_h
