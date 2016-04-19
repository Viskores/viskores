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
#include <vtkm/rendering/BoundingBoxAnnotation.h>

namespace vtkm {
namespace rendering {

#if 0
template<typename SceneType, typename SceneRendererType, typename SurfaceType>
class Window
{
public:
    SceneType scene;
    SceneRendererType sceneRenderer;
    SurfaceType surface;
    vtkm::rendering::Color bgColor;
    vtkm::rendering::View view;
  
    VTKM_CONT_EXPORT
    Window(const SceneType &s,
           const SceneRendererType &sr,
           const SurfaceType &surf,
           const vtkm::rendering::View &v,
           const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        scene(s), sceneRenderer(sr), bgColor(bg), surface(surf), view(v)
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
        
        scene.Render(sceneRenderer, surface, view);
        
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
        surface.SetViewToWorldSpace(view, viewportClip);
    }
};
#endif
    
// Window2D Window3D
template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window3D
{
public:
    Color bgColor;
    vtkm::rendering::Scene3D scene;
    WorldAnnotatorType worldAnnotator;
    SceneRendererType sceneRenderer;
    SurfaceType surface;
    vtkm::rendering::View view;

    // 3D-specific annotations
    BoundingBoxAnnotation bbox;

    VTKM_CONT_EXPORT
    Window3D(const vtkm::rendering::Scene3D &s,
             const SceneRendererType &sr,
             const SurfaceType &surf,
             const vtkm::rendering::View &v,
             const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        scene(s), sceneRenderer(sr), bgColor(bg), surface(surf), view(v)
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
        
        scene.Render(sceneRenderer, surface, view);
        RenderWorldAnnotations();

        surface.Finish();
    }

    VTKM_CONT_EXPORT
    void RenderWorldAnnotations()
    {
        bbox.SetColor(Color(.5,.5,.5));
        bbox.SetExtents(scene.GetSpatialBounds());
        bbox.Render(view, worldAnnotator);
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
        surface.SetViewToWorldSpace(view,viewportClip);
    }
};


template<typename SceneRendererType,
         typename SurfaceType>
class Window2D
{
public:
    Color bgColor;
    vtkm::rendering::Scene2D scene;
    SceneRendererType sceneRenderer;
    SurfaceType surface;
    vtkm::rendering::View view;
  
    VTKM_CONT_EXPORT
    Window2D(const vtkm::rendering::Scene2D &s,
             const SceneRendererType &sr,
             const SurfaceType &surf,
             const vtkm::rendering::View &v,
             const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        scene(s), sceneRenderer(sr), surface(surf), view(v), bgColor(bg)
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
        
        scene.Render(sceneRenderer, surface, view);

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
        surface.SetViewToWorldSpace(view,viewportClip);
    }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Window_h
