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
#include <vtkm/rendering/BoundingBoxAnnotation.h>
#include <vtkm/rendering/AxisAnnotation3D.h>
#include <vtkm/rendering/AxisAnnotation2D.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/ColorBarAnnotation.h>
#include <vtkm/rendering/TextAnnotation.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/View.h>

namespace vtkm {
namespace rendering {

template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window
{
public:
  SceneRendererType SceneRenderer;
  SurfaceType Surface;
  vtkm::rendering::View View;
  Color BackgroundColor;
  WorldAnnotatorType WorldAnnotator;

  Window(const SceneRendererType &sceneRenderer,
         const SurfaceType &surface,
         const vtkm::rendering::View &view,
         const vtkm::rendering::Color &backgroundColor =
           vtkm::rendering::Color(0,0,0,1))
    : SceneRenderer(sceneRenderer),
      Surface(surface),
      View(view),
      BackgroundColor(backgroundColor)
  {
    this->SceneRenderer.SetBackgroundColor(this->BackgroundColor);
  }

  VTKM_CONT_EXPORT
  virtual void Initialize() {this->Surface.Initialize();}

  VTKM_CONT_EXPORT
  virtual void Paint() = 0;
  VTKM_CONT_EXPORT
  virtual void RenderScreenAnnotations() {}
  VTKM_CONT_EXPORT
  virtual void RenderWorldAnnotations() {}

  VTKM_CONT_EXPORT
  void SaveAs(const std::string &fileName)
  {
    this->Surface.SaveAs(fileName);
  }

protected:
  VTKM_CONT_EXPORT
  void SetupForWorldSpace(bool viewportClip=true)
  {
    //this->View.SetupMatrices();
    this->Surface.SetViewToWorldSpace(this->View,viewportClip);
  }

  VTKM_CONT_EXPORT
  void SetupForScreenSpace(bool viewportClip=false)
  {
    //this->View.SetupMatrices();
    this->Surface.SetViewToScreenSpace(this->View,viewportClip);
  }
};

// Window2D Window3D
template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window3D : public Window<SceneRendererType, SurfaceType,WorldAnnotatorType>
{
  typedef Window<SceneRendererType, SurfaceType,WorldAnnotatorType> Superclass;
public:
  vtkm::rendering::Scene3D Scene;
  // 3D-specific annotations
  vtkm::rendering::BoundingBoxAnnotation BoxAnnotation;
  vtkm::rendering::AxisAnnotation3D XAxisAnnotation;
  vtkm::rendering::AxisAnnotation3D YAxisAnnotation;
  vtkm::rendering::AxisAnnotation3D ZAxisAnnotation;
  vtkm::rendering::ColorBarAnnotation ColorBarAnnotation;

  VTKM_CONT_EXPORT
  Window3D(const vtkm::rendering::Scene3D &scene,
           const SceneRendererType &sceneRenderer,
           const SurfaceType &surface,
           const vtkm::rendering::View &view,
           const vtkm::rendering::Color &backgroundColor =
             vtkm::rendering::Color(0,0,0,1))
    : Superclass(sceneRenderer,surface,view,backgroundColor),
      Scene(scene)
  {
  }

  VTKM_CONT_EXPORT
  virtual void Paint()
  {
    this->Surface.Activate();
    this->Surface.Clear();
    this->SetupForWorldSpace();
    this->Scene.Render(this->SceneRenderer, this->Surface, this->View);
    this->RenderWorldAnnotations();

    this->SetupForScreenSpace();
    this->RenderScreenAnnotations();

    this->Surface.Finish();
  }

  VTKM_CONT_EXPORT
  virtual void RenderScreenAnnotations()
  {
    if (this->Scene.Plots.size() > 0)
    {
      //this->ColorBarAnnotation.SetAxisColor(vtkm::rendering::Color(1,1,1));
      this->ColorBarAnnotation.SetRange(this->Scene.Plots[0].ScalarRange, 5);
      this->ColorBarAnnotation.SetColorTable(this->Scene.Plots[0].ColorTable);
      this->ColorBarAnnotation.Render(this->View, this->WorldAnnotator, this->Surface);
    }
  }

  VTKM_CONT_EXPORT
  virtual void RenderWorldAnnotations()
  {
    vtkm::Bounds bounds = this->Scene.GetSpatialBounds();
    vtkm::Float64 xmin = bounds.X.Min, xmax = bounds.X.Max;
    vtkm::Float64 ymin = bounds.Y.Min, ymax = bounds.Y.Max;
    vtkm::Float64 zmin = bounds.Z.Min, zmax = bounds.Z.Max;
    vtkm::Float64 dx = xmax-xmin, dy = ymax-ymin, dz = zmax-zmin;
    vtkm::Float64 size = vtkm::Sqrt(dx*dx + dy*dy + dz*dz);

    this->BoxAnnotation.SetColor(Color(.5f,.5f,.5f));
    this->BoxAnnotation.SetExtents(this->Scene.GetSpatialBounds());
    this->BoxAnnotation.Render(this->View, this->WorldAnnotator);

    bool xtest = this->View.View3d.LookAt[0] > this->View.View3d.Position[0];
    bool ytest = this->View.View3d.LookAt[1] > this->View.View3d.Position[1];
    bool ztest = this->View.View3d.LookAt[2] > this->View.View3d.Position[2];

    const bool outsideedges = true; // if false, do closesttriad
    if (outsideedges)
    {
      xtest = !xtest;
      //ytest = !ytest;
    }

    vtkm::Float64 xrel = vtkm::Abs(dx) / size;
    vtkm::Float64 yrel = vtkm::Abs(dy) / size;
    vtkm::Float64 zrel = vtkm::Abs(dz) / size;

    this->XAxisAnnotation.SetAxis(0);
    this->XAxisAnnotation.SetColor(Color(1,1,1));
    this->XAxisAnnotation.SetTickInvert(xtest,ytest,ztest);
    this->XAxisAnnotation.SetWorldPosition(xmin,
                                           ytest ? ymin : ymax,
                                           ztest ? zmin : zmax,
                                           xmax,
                                           ytest ? ymin : ymax,
                                           ztest ? zmin : zmax);
    this->XAxisAnnotation.SetRange(xmin, xmax);
    this->XAxisAnnotation.SetMajorTickSize(size / 40.f, 0);
    this->XAxisAnnotation.SetMinorTickSize(size / 80.f, 0);
    this->XAxisAnnotation.SetLabelFontOffset(vtkm::Float32(size / 15.f));
    this->XAxisAnnotation.SetMoreOrLessTickAdjustment(xrel < .3 ? -1 : 0);
    this->XAxisAnnotation.Render(this->View, this->WorldAnnotator, this->Surface);

    this->YAxisAnnotation.SetAxis(1);
    this->YAxisAnnotation.SetColor(Color(1,1,1));
    this->YAxisAnnotation.SetTickInvert(xtest,ytest,ztest);
    this->YAxisAnnotation.SetWorldPosition(xtest ? xmin : xmax,
                                           ymin,
                                           ztest ? zmin : zmax,
                                           xtest ? xmin : xmax,
                                           ymax,
                                           ztest ? zmin : zmax);
    this->YAxisAnnotation.SetRange(ymin, ymax);
    this->YAxisAnnotation.SetMajorTickSize(size / 40.f, 0);
    this->YAxisAnnotation.SetMinorTickSize(size / 80.f, 0);
    this->YAxisAnnotation.SetLabelFontOffset(vtkm::Float32(size / 15.f));
    this->YAxisAnnotation.SetMoreOrLessTickAdjustment(yrel < .3 ? -1 : 0);
    this->YAxisAnnotation.Render(this->View, this->WorldAnnotator, this->Surface);

    this->ZAxisAnnotation.SetAxis(2);
    this->ZAxisAnnotation.SetColor(Color(1,1,1));
    this->ZAxisAnnotation.SetTickInvert(xtest,ytest,ztest);
    this->ZAxisAnnotation.SetWorldPosition(xtest ? xmin : xmax,
                                           ytest ? ymin : ymax,
                                           zmin,
                                           xtest ? xmin : xmax,
                                           ytest ? ymin : ymax,
                                           zmax);
    this->ZAxisAnnotation.SetRange(zmin, zmax);
    this->ZAxisAnnotation.SetMajorTickSize(size / 40.f, 0);
    this->ZAxisAnnotation.SetMinorTickSize(size / 80.f, 0);
    this->ZAxisAnnotation.SetLabelFontOffset(vtkm::Float32(size / 15.f));
    this->ZAxisAnnotation.SetMoreOrLessTickAdjustment(zrel < .3 ? -1 : 0);
    this->ZAxisAnnotation.Render(this->View, this->WorldAnnotator, this->Surface);
  }
};

template<typename SceneRendererType,
         typename SurfaceType,
         typename WorldAnnotatorType>
class Window2D : public Window<SceneRendererType, SurfaceType,WorldAnnotatorType>
{
  typedef Window<SceneRendererType, SurfaceType,WorldAnnotatorType> Superclass;
public:
  vtkm::rendering::Scene2D Scene;
  // 2D-specific annotations
  vtkm::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  vtkm::rendering::AxisAnnotation2D VerticalAxisAnnotation;
  vtkm::rendering::ColorBarAnnotation ColorBarAnnotation;

  VTKM_CONT_EXPORT
  Window2D(const vtkm::rendering::Scene2D &scene,
           const SceneRendererType &sceneRenderer,
           const SurfaceType &surface,
           const vtkm::rendering::View &view,
           const vtkm::rendering::Color &backgroundColor =
             vtkm::rendering::Color(0,0,0,1))
    : Superclass(sceneRenderer, surface, view, backgroundColor),
      Scene(scene)
  {
  }

  VTKM_CONT_EXPORT
  virtual void Paint()
  {
    this->Surface.Activate();
    this->Surface.Clear();
    this->SetupForWorldSpace();

    this->Scene.Render(this->SceneRenderer, this->Surface, this->View);
    this->RenderWorldAnnotations();

    this->SetupForScreenSpace();
    this->RenderScreenAnnotations();

    this->Surface.Finish();
  }

  VTKM_CONT_EXPORT
  void RenderScreenAnnotations()
  {
    vtkm::Float32 viewportLeft;
    vtkm::Float32 viewportRight;
    vtkm::Float32 viewportTop;
    vtkm::Float32 viewportBottom;
    this->View.GetRealViewport(
          viewportLeft, viewportRight, viewportBottom, viewportTop);

    this->HorizontalAxisAnnotation.SetColor(vtkm::rendering::Color(1,1,1));
    this->HorizontalAxisAnnotation.SetScreenPosition(
          viewportLeft, viewportBottom, viewportRight, viewportBottom);
    this->HorizontalAxisAnnotation.SetRangeForAutoTicks(this->View.View2d.Left,
                                                        this->View.View2d.Right);
    this->HorizontalAxisAnnotation.SetMajorTickSize(0, .05, 1.0);
    this->HorizontalAxisAnnotation.SetMinorTickSize(0, .02, 1.0);
    this->HorizontalAxisAnnotation.SetLabelAlignment(TextAnnotation::HCenter,
                                                     TextAnnotation::Top);
    this->HorizontalAxisAnnotation.Render(
          this->View, this->WorldAnnotator, this->Surface);

    vtkm::Float32 windowaspect =
        vtkm::Float32(this->View.Width) / vtkm::Float32(this->View.Height);

    this->VerticalAxisAnnotation.SetColor(Color(1,1,1));
    this->VerticalAxisAnnotation.SetScreenPosition(
          viewportLeft, viewportBottom, viewportLeft, viewportTop);
    this->VerticalAxisAnnotation.SetRangeForAutoTicks(this->View.View2d.Bottom,
                                                      this->View.View2d.Top);
    this->VerticalAxisAnnotation.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
    this->VerticalAxisAnnotation.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
    this->VerticalAxisAnnotation.SetLabelAlignment(TextAnnotation::Right,
                                                   TextAnnotation::VCenter);
    this->VerticalAxisAnnotation.Render(
          this->View, this->WorldAnnotator, this->Surface);

    if (this->Scene.Plots.size() > 0)
    {
      //this->ColorBarAnnotation.SetAxisColor(vtkm::rendering::Color(1,1,1));
      this->ColorBarAnnotation.SetRange(this->Scene.Plots[0].ScalarRange.Min,
                                        this->Scene.Plots[0].ScalarRange.Max,
                                        5);
      this->ColorBarAnnotation.SetColorTable(this->Scene.Plots[0].ColorTable);
      this->ColorBarAnnotation.Render(
            this->View, this->WorldAnnotator, this->Surface);
    }
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Window_h
