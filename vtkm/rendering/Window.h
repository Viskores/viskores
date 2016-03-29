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
#ifndef vtk_m_rendering_Window_h
#define vtk_m_rendering_Window_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>

namespace vtkm {
namespace rendering {

// Window2D Window3D
template<typename SceneRendererType, typename SurfaceType>
class Window3D
{
public:
    Color bgColor;
    vtkm::rendering::Scene3D scene;
    SceneRendererType sceneRenderer;
    SurfaceType surface;
    //vtkm::rendering::View3D view;
  
    VTKM_CONT_EXPORT
    Window3D(const vtkm::rendering::Scene3D &s,
             const SceneRendererType &sr,
             const SurfaceType &surf,
             const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        scene(s), sceneRenderer(sr), bgColor(bg), surface(surf)
    {
        sceneRenderer.SetBackgroundColor(bgColor);
    }

    VTKM_CONT_EXPORT
    void Initialize()
    {
        surface.Initialize();
    }

    VTKM_CONT_EXPORT
    void Paint()
    {
        surface.Activate();
        surface.Clear();
        SetupForWorldSpace();
        
        scene.Render(sceneRenderer, surface);
        
        surface.Finish();
    }

    VTKM_CONT_EXPORT
    void SaveAs(const std::string &fileName)
    {
        surface.SaveAs(fileName);
    }

private:
    VTKM_CONT_EXPORT
    void SetupForWorldSpace(bool viewportClip=true)
    {
        //view.SetupMatrices();
        surface.SetViewToWorldSpace(sceneRenderer.GetView(),
                                    viewportClip);
    }
};


template<typename SceneRendererType>
class Window2D
{
public:
    Color bgColor;
    vtkm::rendering::Scene2D scene;
    SceneRendererType sceneRenderer;
    vtkm::rendering::View2D view;
  
    VTKM_CONT_EXPORT
    Window2D(const vtkm::rendering::Scene2D &s,
             const SceneRendererType &sr,
             const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        scene(s), sceneRenderer(sr), bgColor(bg)
    {
    }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Window_h
