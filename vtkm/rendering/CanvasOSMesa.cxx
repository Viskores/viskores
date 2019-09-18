//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/CanvasOSMesa.h>

#include <vtkm/Types.h>
#include <vtkm/rendering/CanvasGL.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>
#ifndef GLAPI
#define GLAPI extern
#endif

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifndef APIENTRY
#define APIENTRY GLAPIENTRY
#endif
#include <GL/osmesa.h>

namespace vtkm
{
namespace rendering
{

namespace detail
{

struct CanvasOSMesaInternals
{
  OSMesaContext Context;
};

} // namespace detail

CanvasOSMesa::CanvasOSMesa(vtkm::Id width, vtkm::Id height)
  : CanvasGL(width, height)
  , Internals(new detail::CanvasOSMesaInternals)
{
  this->Internals->Context = nullptr;
  this->ResizeBuffers(width, height);
}

CanvasOSMesa::~CanvasOSMesa()
{
}

void CanvasOSMesa::Initialize()
{
  this->Internals->Context = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, nullptr);

  if (!this->Internals->Context)
  {
    throw vtkm::cont::ErrorBadValue("OSMesa context creation failed.");
  }
  vtkm::Vec4f_32* colorBuffer = this->GetColorBuffer().GetStorage().GetArray();
  if (!OSMesaMakeCurrent(this->Internals->Context,
                         reinterpret_cast<vtkm::Float32*>(colorBuffer),
                         GL_FLOAT,
                         static_cast<GLsizei>(this->GetWidth()),
                         static_cast<GLsizei>(this->GetHeight())))
  {
    throw vtkm::cont::ErrorBadValue("OSMesa context activation failed.");
  }
}

void CanvasOSMesa::RefreshColorBuffer() const
{
  // Override superclass because our OSMesa implementation renders right
  // to the color buffer.
}

void CanvasOSMesa::Activate()
{
  glEnable(GL_DEPTH_TEST);
}

void CanvasOSMesa::Finish()
{
  this->CanvasGL::Finish();

// This is disabled because it is handled in RefreshDepthBuffer
#if 0
  //Copy zbuff into floating point array.
  unsigned int *raw_zbuff;
  int zbytes, w, h;
  GLboolean ret;
  ret = OSMesaGetDepthBuffer(this->Internals->Context, &w, &h, &zbytes, (void**)&raw_zbuff);
  if (!ret ||
      static_cast<vtkm::Id>(w)!=this->GetWidth() ||
      static_cast<vtkm::Id>(h)!=this->GetHeight())
  {
    throw vtkm::cont::ErrorBadValue("Wrong width/height in ZBuffer");
  }
  vtkm::cont::ArrayHandle<vtkm::Float32>::PortalControl depthPortal =
      this->GetDepthBuffer().GetPortalControl();
  vtkm::Id npixels = this->GetWidth()*this->GetHeight();
  for (vtkm::Id i=0; i<npixels; i++)
  for (std::size_t i=0; i<npixels; i++)
  {
    depthPortal.Set(i, float(raw_zbuff[i]) / float(UINT_MAX));
  }
#endif
}

vtkm::rendering::Canvas* CanvasOSMesa::NewCopy() const
{
  return new vtkm::rendering::CanvasOSMesa(*this);
}
}
} // namespace vtkm::rendering
