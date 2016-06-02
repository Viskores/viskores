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
#ifndef vtk_m_rendering_RenderSurfaceGLX_h
#define vtk_m_rendering_RenderSurfaceGLX_h

#include <vtkm/Types.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/RenderSurfaceGL.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class RenderSurfaceGLX : public RenderSurfaceGL
{
public:
  VTKM_CONT_EXPORT
  RenderSurfaceGLX(std::size_t w=1024, std::size_t h=1024,
                   const vtkm::rendering::Color &c=vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
    : RenderSurfaceGL(w,h,c)
  {
    ctx = NULL;
  }

  VTKM_CONT_EXPORT
  virtual void Initialize()
  {
    ctx = glXGetCurrentContext();
    if (!ctx)
      throw vtkm::cont::ErrorControlBadValue("GL context creation failed.");
    /*
      rgba.resize(width*height*4);
      if (!OSMesaMakeCurrent(ctx, &rgba[0], GL_FLOAT, static_cast<GLsizei>(width), static_cast<GLsizei>(height)))
      throw vtkm::cont::ErrorControlBadValue("OSMesa context activation failed.");
    */

    glEnable(GL_DEPTH_TEST);
  }

  VTKM_CONT_EXPORT
  virtual void Clear()
  {
    glClearColor(this->BackgroundColor.Components[0],
                 this->BackgroundColor.Components[1],
                 this->BackgroundColor.Components[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  VTKM_CONT_EXPORT
  virtual void Finish()
  {
    RenderSurfaceGL::Finish();


    /* TODO
    //Copy zbuff into floating point array.
    unsigned int *raw_zbuff;
    int zbytes, w, h;
    GLboolean ret;
    ret = OSMesaGetDepthBuffer(ctx, &w, &h, &zbytes, (void**)&raw_zbuff);
    if (!ret || static_cast<std::size_t>(w)!=width || static_cast<std::size_t>(h)!=height)
    throw vtkm::cont::ErrorControlBadValue("Wrong width/height in ZBuffer");
    std::size_t npixels = width*height;
    for (std::size_t i=0; i<npixels; i++)
    zbuff[i] = float(raw_zbuff[i]) / float(UINT_MAX);
    */
  }

private:
  GLXContext ctx;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurfaceGLX_h
