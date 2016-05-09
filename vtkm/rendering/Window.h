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
#include <vtkm/rendering/AxisAnnotation3D.h>
#include <vtkm/rendering/AxisAnnotation2D.h>

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
    AxisAnnotation3D xaxis, yaxis, zaxis;

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

        SetupForScreenSpace();
        RenderScreenAnnotations();

        surface.Finish();
    }

    VTKM_CONT_EXPORT
    void RenderScreenAnnotations()
    {
    }

    VTKM_CONT_EXPORT
    void RenderWorldAnnotations()
    {
        double *bnd = scene.GetSpatialBounds();
        double xmin = bnd[0], xmax = bnd[1];
        double ymin = bnd[2], ymax = bnd[3];
        double zmin = bnd[4], zmax = bnd[5];
        double dx = xmax-xmin, dy = ymax-ymin, dz = zmax-zmin;
        double size = sqrt(dx*dx + dy*dy + dz*dz);

        bbox.SetColor(Color(.5,.5,.5));
        bbox.SetExtents(scene.GetSpatialBounds());
        bbox.Render(view, worldAnnotator);

        ///\todo: set x/y/ztest based on view
        bool xtest=true, ytest=false, ztest=false;

        double xrel = fabs(dx) / size;
        double yrel = fabs(dy) / size;
        double zrel = fabs(dz) / size;

        xaxis.SetAxis(0);
        xaxis.SetColor(Color(1,1,1));
        xaxis.SetTickInvert(xtest,ytest,ztest);
        xaxis.SetWorldPosition(xmin,
                               ytest ? ymin : ymax,
                               ztest ? zmin : zmax,
                               xmax,
                               ytest ? ymin : ymax,
                               ztest ? zmin : zmax);
        xaxis.SetRange(xmin, xmax);
        xaxis.SetMajorTickSize(size / 40.f, 0);
        xaxis.SetMinorTickSize(size / 80.f, 0);
        xaxis.SetLabelFontScale(size / 30.);
        xaxis.SetMoreOrLessTickAdjustment(xrel < .3 ? -1 : 0);
        xaxis.Render(view, worldAnnotator);

        yaxis.SetAxis(0);
        yaxis.SetColor(Color(1,1,1));
        yaxis.SetTickInvert(xtest,ytest,ztest);
        yaxis.SetWorldPosition(xtest ? xmin : xmax,
                               ymin,
                               ztest ? zmin : zmax,
                               xtest ? xmin : xmax,
                               ymax,
                               ztest ? zmin : zmax);
        yaxis.SetRange(ymin, ymax);
        yaxis.SetMajorTickSize(size / 40.f, 0);
        yaxis.SetMinorTickSize(size / 80.f, 0);
        yaxis.SetLabelFontScale(size / 30.);
        yaxis.SetMoreOrLessTickAdjustment(yrel < .3 ? -1 : 0);
        yaxis.Render(view, worldAnnotator);

        zaxis.SetAxis(0);
        zaxis.SetColor(Color(1,1,1));
        zaxis.SetTickInvert(xtest,ytest,ztest);
        zaxis.SetWorldPosition(xtest ? xmin : xmax,
                               ytest ? ymin : ymax,
                               zmin,
                               xtest ? xmin : xmax,
                               ytest ? ymin : ymax,
                               zmax);
        zaxis.SetRange(zmin, zmax);
        zaxis.SetMajorTickSize(size / 40.f, 0);
        zaxis.SetMinorTickSize(size / 80.f, 0);
        zaxis.SetLabelFontScale(size / 30.);
        zaxis.SetMoreOrLessTickAdjustment(zrel < .3 ? -1 : 0);
        zaxis.Render(view, worldAnnotator);
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

    VTKM_CONT_EXPORT
    void SetupForScreenSpace(bool viewportClip=false)
    {
        //view.SetupMatrices();
        surface.SetViewToScreenSpace(view,viewportClip);
    }
};

#include <GL/osmesa.h>
#include <GL/gl.h>

template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window2D
{
public:
    Color bgColor;
    vtkm::rendering::Scene2D scene;
    WorldAnnotatorType worldAnnotator;
    SceneRendererType sceneRenderer;
    SurfaceType surface;
    vtkm::rendering::View view;

    // 2D-specific annotations
    AxisAnnotation2D haxis, vaxis;

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
        RenderWorldAnnotations();

        SetupForScreenSpace();
        RenderScreenAnnotations();

        surface.Finish();
    }

    VTKM_CONT_EXPORT
    void RenderScreenAnnotations()
    {
        vtkm::Float32 vl, vr, vt, vb;
        view.GetRealViewport(vl,vr,vb,vt);

        haxis.SetColor(Color(1,1,1));
        haxis.SetScreenPosition(vl,vb, vr,vb);
        haxis.SetRangeForAutoTicks(view.view2d.left, view.view2d.right);
        haxis.SetMajorTickSize(0, .05, 1.0);
        haxis.SetMinorTickSize(0, .02, 1.0);
        //haxis.SetLabelAlignment(eavlTextAnnotation::HCenter,
        //                         eavlTextAnnotation::Top);
        haxis.Render(view, worldAnnotator, surface);

        vtkm::Float32 windowaspect = vtkm::Float32(view.width) / vtkm::Float32(view.height);

        vaxis.SetColor(Color(1,1,1));
        vaxis.SetScreenPosition(vl,vb, vl,vt);
        vaxis.SetRangeForAutoTicks(view.view2d.bottom, view.view2d.top);
        vaxis.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
        vaxis.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
        //vaxis.SetLabelAlignment(eavlTextAnnotation::Right,
        //                         eavlTextAnnotation::VCenter);
        vaxis.Render(view, worldAnnotator, surface);
    }

    VTKM_CONT_EXPORT
    void RenderWorldAnnotations()
    {
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

    VTKM_CONT_EXPORT
    void SetupForScreenSpace(bool viewportClip=false)
    {
        //view.SetupMatrices();
        surface.SetViewToScreenSpace(view,viewportClip);
    }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Window_h
