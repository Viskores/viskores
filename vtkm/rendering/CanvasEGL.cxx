//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/CanvasEGL.h>

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>

#include <EGL/egl.h>
//#include <GL/gl.h>

namespace vtkm
{
namespace rendering
{

namespace detail
{

struct CanvasEGLInternals
{
  EGLContext Context;
  EGLDisplay Display;
  EGLSurface Surface;
};

} // namespace detail

CanvasEGL::CanvasEGL(vtkm::Id width, vtkm::Id height)
  : CanvasGL(width, height)
  , Internals(new detail::CanvasEGLInternals)
{
  this->Internals->Context = nullptr;
  this->ResizeBuffers(width, height);
}

CanvasEGL::~CanvasEGL()
{
}

void CanvasEGL::Initialize()
{
  this->Internals->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (!(this->Internals->Display))
  {
    throw vtkm::cont::ErrorBadValue("Failed to get EGL display");
  }
  EGLint major, minor;
  if (!(eglInitialize(this->Internals->Display, &major, &minor)))
  {
    throw vtkm::cont::ErrorBadValue("Failed to initialize EGL display");
  }

  const EGLint cfgAttrs[] = { EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              8,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_BIT,
                              EGL_NONE };

  EGLint nCfgs;
  EGLConfig cfg;
  if (!(eglChooseConfig(this->Internals->Display, cfgAttrs, &cfg, 1, &nCfgs)) || (nCfgs == 0))
  {
    throw vtkm::cont::ErrorBadValue("Failed to get EGL config");
  }

  const EGLint pbAttrs[] = {
    EGL_WIDTH,  static_cast<EGLint>(this->GetWidth()),
    EGL_HEIGHT, static_cast<EGLint>(this->GetHeight()),
    EGL_NONE,
  };

  this->Internals->Surface = eglCreatePbufferSurface(this->Internals->Display, cfg, pbAttrs);
  if (!this->Internals->Surface)
  {
    throw vtkm::cont::ErrorBadValue("Failed to create EGL PBuffer surface");
  }
  eglBindAPI(EGL_OPENGL_API);
  this->Internals->Context =
    eglCreateContext(this->Internals->Display, cfg, EGL_NO_CONTEXT, nullptr);
  if (!this->Internals->Context)
  {
    throw vtkm::cont::ErrorBadValue("Failed to create EGL context");
  }
  if (!(eglMakeCurrent(this->Internals->Display,
                       this->Internals->Surface,
                       this->Internals->Surface,
                       this->Internals->Context)))
  {
    throw vtkm::cont::ErrorBadValue("Failed to create EGL context current");
  }
}

void CanvasEGL::Activate()
{
  glEnable(GL_DEPTH_TEST);
}

vtkm::rendering::Canvas* CanvasEGL::NewCopy() const
{
  return new vtkm::rendering::CanvasEGL(*this);
}
}
} // namespace vtkm::rendering
