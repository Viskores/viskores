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
#ifndef vtk_m_rendering_CanvasEGL_h
#define vtk_m_rendering_CanvasEGL_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/Color.h>

#include <EGL/egl.h>
//#include <GL/gl.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class CanvasEGL : public CanvasGL
{
public:
  VTKM_CONT_EXPORT
  CanvasEGL(vtkm::Id width=1024,
            vtkm::Id height=1024)
    : CanvasGL()
  {
    ctx = NULL;
    this->ResizeBuffers(width, height);
  }

  VTKM_CONT_EXPORT
  virtual void Initialize()
  {
    if (!(dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY)))
      throw vtkm::cont::ErrorControlBadValue("Failed to get EGL display");
    EGLint major, minor;
    if (!(eglInitialize(dpy, &major, &minor)))
      throw vtkm::cont::ErrorControlBadValue("Failed to initialize EGL display");        
    
    const EGLint cfgAttrs[] =
    {
      EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
      EGL_BLUE_SIZE, 8,
      EGL_GREEN_SIZE, 8,
      EGL_RED_SIZE, 8,
      EGL_DEPTH_SIZE, 8,
      EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
      EGL_NONE
    };
    
    EGLint nCfgs;
    EGLConfig cfg;
    if (!(eglChooseConfig(dpy, cfgAttrs, &cfg, 1, &nCfgs)) || nCfgs == 0)
      throw vtkm::cont::ErrorControlBadValue("Failed to get EGL config");

    const EGLint pbAttrs[] =
    {
      EGL_WIDTH, static_cast<EGLint>(this->GetWidth()),
      EGL_HEIGHT, static_cast<EGLint>(this->GetHeight()),
      EGL_NONE,
    };
    
    if (!(surf = eglCreatePbufferSurface(dpy, cfg, pbAttrs)))
      throw vtkm::cont::ErrorControlBadValue("Failed to create EGL PBuffer surface");
    eglBindAPI(EGL_OPENGL_API);
    if (!(ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, NULL)))
      throw vtkm::cont::ErrorControlBadValue("Failed to create EGL context");
    if (!(eglMakeCurrent(dpy, surf, surf, ctx)))
      throw vtkm::cont::ErrorControlBadValue("Failed to create EGL context current");
  }
    
  VTKM_CONT_EXPORT
  virtual void Activate()
  {
    glEnable(GL_DEPTH_TEST);
  }

  VTKM_CONT_EXPORT
  virtual void Clear()
  {
    vtkm::rendering::Color backgroundColor = this->GetBackgroundColor();
    glClearColor(backgroundColor.Components[0],
                 backgroundColor.Components[1],
                 backgroundColor.Components[2],
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

private:
  EGLContext ctx;
  EGLDisplay dpy;
  EGLSurface surf;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasEGL_h
