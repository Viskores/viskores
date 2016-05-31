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
#ifndef vtk_m_rendering_CanvasOSMesa_h
#define vtk_m_rendering_CanvasOSMesa_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/Color.h>

#include <GL/osmesa.h>
#include <GL/gl.h>
#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class CanvasOSMesa : public CanvasGL
{
public:
  VTKM_CONT_EXPORT
  CanvasOSMesa(vtkm::Id w=1024,
               vtkm::Id h=1024,
               const vtkm::rendering::Color &c =
                 vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
    : CanvasGL(w,h,c)
  {
    ctx = NULL;
  }

  VTKM_CONT_EXPORT
  virtual void Initialize()
  {
    ctx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, NULL);
    if (!ctx)
    {
      throw vtkm::cont::ErrorControlBadValue("OSMesa context creation failed.");
    }
    vtkm::Vec<vtkm::Float32,4> *colorBuffer =
        vtkm::cont::ArrayPortalToIteratorBegin(this->ColorBuffer.GetPortalControl());
    if (!OSMesaMakeCurrent(ctx,
                           reinterpret_cast<vtkm::Float32*>(colorBuffer),
                           GL_FLOAT,
                           static_cast<GLsizei>(this->Width),
                           static_cast<GLsizei>(this->Height)))
    {
      throw vtkm::cont::ErrorControlBadValue("OSMesa context activation failed.");
    }
  }

  VTKM_CONT_EXPORT
  virtual void RefreshColorBuffer()
  {
    // Override superclass because our OSMesa implementation renders right
    // to the color buffer.
  }

  VTKM_CONT_EXPORT
  virtual void Activate()
  {
    glEnable(GL_DEPTH_TEST);
  }

  VTKM_CONT_EXPORT
  virtual void Clear()
  {
    glClearColor(this->BackgroundColor.Components[0],
                 this->BackgroundColor.Components[1],
                 this->BackgroundColor.Components[2],
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }
  VTKM_CONT_EXPORT
  virtual void Finish()
  {
    CanvasGL::Finish();

    // This is disabled because it is handled in RefreshDepthBuffer
#if 0
    //Copy zbuff into floating point array.
    unsigned int *raw_zbuff;
    int zbytes, w, h;
    GLboolean ret;
    ret = OSMesaGetDepthBuffer(ctx, &w, &h, &zbytes, (void**)&raw_zbuff);
    if (!ret ||
        static_cast<vtkm::Id>(w)!=this->Width ||
        static_cast<vtkm::Id>(h)!=this->Height)
    {
      throw vtkm::cont::ErrorControlBadValue("Wrong width/height in ZBuffer");
    }
    vtkm::cont::ArrayHandle<vtkm::Float32>::PortalControl depthPortal =
        this->DepthBuffer.GetPortalControl();
    vtkm::Id npixels = this->Width*this->Height;
    for (vtkm::Id i=0; i<npixels; i++)
    for (std::size_t i=0; i<npixels; i++)
    {
      depthPortal.Set(i, float(raw_zbuff[i]) / float(UINT_MAX));
    }
#endif
  }

private:
  OSMesaContext ctx;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasOSMesa_h
