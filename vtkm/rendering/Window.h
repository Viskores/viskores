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
#include <vtkm/rendering/ColorBarAnnotation.h>

namespace vtkm {
namespace rendering {

template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window
{
public:
    SceneRendererType sceneRenderer;
    SurfaceType surface;
    Color bgColor;
    vtkm::rendering::View view;
    WorldAnnotatorType worldAnnotator;

    Window(const SceneRendererType &sr,
           const SurfaceType &surf,
           const vtkm::rendering::View &v,
           const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        bgColor(bg), view(v), sceneRenderer(sr), surface(surf)
    {
        sceneRenderer.SetBackgroundColor(bgColor);
    }

    VTKM_CONT_EXPORT
    virtual void Initialize() {surface.Initialize();}

    VTKM_CONT_EXPORT
    virtual void Paint() = 0;
    VTKM_CONT_EXPORT
    virtual void RenderScreenAnnotations() {}
    VTKM_CONT_EXPORT
    virtual void RenderWorldAnnotations() {}

    VTKM_CONT_EXPORT
    void SaveAs(const std::string &fileName)
    {
        surface.SaveAs(fileName);
    }

protected:
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

// Window2D Window3D
template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window3D : public Window<SceneRendererType, SurfaceType,WorldAnnotatorType>
{
public:
    vtkm::rendering::Scene3D scene;
    // 3D-specific annotations
    BoundingBoxAnnotation bbox;
    AxisAnnotation3D xaxis, yaxis, zaxis;
    ColorBarAnnotation colorbar;

    VTKM_CONT_EXPORT
    Window3D(const vtkm::rendering::Scene3D &s,
             const SceneRendererType &sr,
             const SurfaceType &surf,
             const vtkm::rendering::View &v,
             const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        Window<SceneRendererType,SurfaceType,WorldAnnotatorType>(sr,surf,v,bg), scene(s)
    {
    }

    VTKM_CONT_EXPORT
    virtual void Paint()
    {
        this->surface.Activate();
        this->surface.Clear();
        this->SetupForWorldSpace();
        scene.Render(this->sceneRenderer, this->surface, this->view);
        RenderWorldAnnotations();

        this->SetupForScreenSpace();
        RenderScreenAnnotations();
        
        this->surface.Finish();
    }

    VTKM_CONT_EXPORT
    virtual void RenderScreenAnnotations()
    {
        if (scene.plots.size() > 0)
        {
            //colorbar.SetAxisColor(eavlColor::white);
            colorbar.SetRange(scene.plots[0].scalarBounds[0], scene.plots[0].scalarBounds[1], 5);
            colorbar.SetColorTable(scene.plots[0].colorTable);
            colorbar.Render(this->view, this->worldAnnotator, this->surface);
        }
    }

    VTKM_CONT_EXPORT
    virtual void RenderWorldAnnotations()
    {
        vtkm::Float64 *bnd = scene.GetSpatialBounds();
        vtkm::Float64 xmin = bnd[0], xmax = bnd[1];
        vtkm::Float64 ymin = bnd[2], ymax = bnd[3];
        vtkm::Float64 zmin = bnd[4], zmax = bnd[5];
        vtkm::Float64 dx = xmax-xmin, dy = ymax-ymin, dz = zmax-zmin;
        vtkm::Float64 size = vtkm::Sqrt(dx*dx + dy*dy + dz*dz);

        bbox.SetColor(Color(.5f,.5f,.5f));
        bbox.SetExtents(scene.GetSpatialBounds());
        bbox.Render(this->view, this->worldAnnotator);

        ///\todo: set x/y/ztest based on view
        bool xtest=true, ytest=false, ztest=false;

        vtkm::Float64 xrel = vtkm::Abs(dx) / size;
        vtkm::Float64 yrel = vtkm::Abs(dy) / size;
        vtkm::Float64 zrel = vtkm::Abs(dz) / size;

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
        xaxis.Render(this->view, this->worldAnnotator);

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
        yaxis.Render(this->view, this->worldAnnotator);

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
        zaxis.Render(this->view, this->worldAnnotator);
    }
};

template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window2D : public Window<SceneRendererType, SurfaceType,WorldAnnotatorType>
{
public:
    vtkm::rendering::Scene2D scene;
    // 2D-specific annotations
    AxisAnnotation2D haxis, vaxis;
    ColorBarAnnotation colorbar;

    VTKM_CONT_EXPORT
    Window2D(const vtkm::rendering::Scene2D &s,
             const SceneRendererType &sr,
             const SurfaceType &surf,
             const vtkm::rendering::View &v,
             const vtkm::rendering::Color &bg=vtkm::rendering::Color(0,0,0,1)) :
        Window<SceneRendererType,SurfaceType,WorldAnnotatorType>(sr,surf,v,bg), scene(s)        
    {
    }
    
    VTKM_CONT_EXPORT
    virtual void Paint()
    {
        this->surface.Activate();
        this->surface.Clear();
        this->SetupForWorldSpace();
        
        scene.Render(this->sceneRenderer, this->surface, this->view);
        this->RenderWorldAnnotations();

        this->SetupForScreenSpace();
        this->RenderScreenAnnotations();

        this->surface.Finish();
    }

    VTKM_CONT_EXPORT
    void RenderScreenAnnotations()
    {
        vtkm::Float32 vl, vr, vt, vb;
        this->view.GetRealViewport(vl,vr,vb,vt);

        haxis.SetColor(Color(1,1,1));
        haxis.SetScreenPosition(vl,vb, vr,vb);
        haxis.SetRangeForAutoTicks(this->view.View2d.Left, this->view.View2d.Right);
        haxis.SetMajorTickSize(0, .05, 1.0);
        haxis.SetMinorTickSize(0, .02, 1.0);
        //haxis.SetLabelAlignment(eavlTextAnnotation::HCenter,
        //                         eavlTextAnnotation::Top);
        haxis.Render(this->view, this->worldAnnotator, this->surface);

        vtkm::Float32 windowaspect = vtkm::Float32(this->view.Width) / vtkm::Float32(this->view.Height);

        vaxis.SetColor(Color(1,1,1));
        vaxis.SetScreenPosition(vl,vb, vl,vt);
        vaxis.SetRangeForAutoTicks(this->view.View2d.Bottom, this->view.View2d.Top);
        vaxis.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
        vaxis.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
        //vaxis.SetLabelAlignment(eavlTextAnnotation::Right,
        //                         eavlTextAnnotation::VCenter);
        vaxis.Render(this->view, this->worldAnnotator, this->surface);

        if (scene.plots.size() > 0)
        {
            //colorbar.SetAxisColor(eavlColor::white);
            colorbar.SetRange(scene.plots[0].scalarBounds[0], scene.plots[0].scalarBounds[1], 5);
            colorbar.SetColorTable(scene.plots[0].colorTable);
            colorbar.Render(this->view, this->worldAnnotator, this->surface);
        }
    }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Window_h
