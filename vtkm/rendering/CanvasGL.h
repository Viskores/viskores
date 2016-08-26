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
#ifndef vtk_m_rendering_CanvasGL_h
#define vtk_m_rendering_CanvasGL_h

#include <vtkm/Types.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/DecodePNG.h>
#include <vtkm/rendering/TextureGL.h>
#include <vtkm/rendering/MatrixHelpers.h>
#include <vtkm/rendering/WorldAnnotatorGL.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>

#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class CanvasGL : public Canvas
{
public:
  VTKM_CONT_EXPORT
  CanvasGL(vtkm::Id width=1024,
           vtkm::Id height=1024)
    : Canvas(width,height)
  {
  }

  VTKM_CONT_EXPORT
  virtual void Initialize()
  {
    // Nothing to initialize
  }

  VTKM_CONT_EXPORT
  virtual void Activate()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    this->ResizeBuffers(viewport[2], viewport[3]);

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

  VTKM_CONT_EXPORT
  virtual void Finish()
  {
    glFinish();
  }

  VTKM_CONT_EXPORT
  virtual void SetViewToWorldSpace(const vtkm::rendering::Camera &camera,
                                   bool clip)
  {
    vtkm::Float32 oglP[16], oglM[16];

    MatrixHelpers::CreateOGLMatrix(
          camera.CreateProjectionMatrix(this->GetWidth(),
                                        this->GetHeight()),
          oglP);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(oglP);
    MatrixHelpers::CreateOGLMatrix(camera.CreateViewMatrix(), oglM);
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(oglM);

    SetViewportClipping(camera, clip);
  }

  VTKM_CONT_EXPORT
  virtual void SetViewToScreenSpace(const vtkm::rendering::Camera &camera,
                                    bool clip)
  {
    vtkm::Float32 oglP[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
    vtkm::Float32 oglM[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};

    oglP[0*4+0] = 1.;
    oglP[1*4+1] = 1.;
    oglP[2*4+2] = -1.;
    oglP[3*4+3] = 1.;

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(oglP);

    oglM[0*4+0] = 1.;
    oglM[1*4+1] = 1.;
    oglM[2*4+2] = 1.;
    oglM[3*4+3] = 1.;

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(oglM);

    SetViewportClipping(camera, clip);
  }

  VTKM_CONT_EXPORT
  virtual void SetViewportClipping(
      const vtkm::rendering::Camera &camera, bool clip)
  {
    if (clip)
    {
      vtkm::Float32 vl, vr, vb, vt;
      camera.GetRealViewport(this->GetWidth(), this->GetHeight(),
                             vl,vr,vb,vt);
      vtkm::Float32 _x = static_cast<vtkm::Float32>(this->GetWidth())*(1.f+vl)/2.f;
      vtkm::Float32 _y = static_cast<vtkm::Float32>(this->GetHeight())*(1.f+vb)/2.f;
      vtkm::Float32 _w = static_cast<vtkm::Float32>(this->GetWidth())*(vr-vl)/2.f;
      vtkm::Float32 _h = static_cast<vtkm::Float32>(this->GetHeight())*(vt-vb)/2.f;

      glViewport(static_cast<GLint>(_x), static_cast<GLint>(_y),
                 static_cast<GLsizei>(_w), static_cast<GLsizei>(_h));
    }
    else
    {
      glViewport(0,
                 0,
                 static_cast<GLsizei>(this->GetWidth()),
                 static_cast<GLsizei>(this->GetHeight()));
    }
  }

  VTKM_CONT_EXPORT
  virtual void RefreshColorBuffer()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    VTKM_ASSERT(viewport[2] == this->GetWidth());
    VTKM_ASSERT(viewport[3] == this->GetHeight());

    glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3],
        GL_RGBA, GL_FLOAT,
        &(*vtkm::cont::ArrayPortalToIteratorBegin(
            this->GetColorBuffer().GetPortalControl())));
  }
  VTKM_CONT_EXPORT
  virtual void RefreshDepthBuffer()
  {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    VTKM_ASSERT(viewport[2] == this->GetWidth());
    VTKM_ASSERT(viewport[3] == this->GetHeight());

    glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3],
        GL_DEPTH_COMPONENT, GL_FLOAT,
        &(*vtkm::cont::ArrayPortalToIteratorBegin(
            this->GetDepthBuffer().GetPortalControl())));
  }

  VTKM_CONT_EXPORT
  virtual void AddLine(vtkm::Float64 x0, vtkm::Float64 y0,
                       vtkm::Float64 x1, vtkm::Float64 y1,
                       vtkm::Float32 linewidth,
                       const vtkm::rendering::Color &c) const
  {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glColor3f(c.Components[0], c.Components[1], c.Components[2]);

    glLineWidth(linewidth);

    glBegin(GL_LINES);
    glVertex2f(float(x0),float(y0));
    glVertex2f(float(x1),float(y1));
    glEnd();
  }

  VTKM_CONT_EXPORT
  virtual void AddColorBar(vtkm::Float32 x, vtkm::Float32 y,
                           vtkm::Float32 w, vtkm::Float32 h,
                           const vtkm::rendering::ColorTable &ct,
                           bool horizontal) const
  {
    const int n = 256;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    for (int i=0; i<n; i++)
    {
      vtkm::Float32 v0 = static_cast<vtkm::Float32>(i)/static_cast<vtkm::Float32>(n);
      vtkm::Float32 v1 = static_cast<vtkm::Float32>(i+1)/static_cast<vtkm::Float32>(n);
      Color c0 = ct.MapRGB(v0);
      Color c1 = ct.MapRGB(v1);
      if (horizontal)
      {
        vtkm::Float32 x0 = x + w*v0;
        vtkm::Float32 x1 = x + w*v1;
        vtkm::Float32 y0 = y;
        vtkm::Float32 y1 = y + h;
        glColor3f(c0.Components[0], c0.Components[1], c0.Components[2]);
        glVertex2f(x0,y0);
        glVertex2f(x0,y1);
        glColor3f(c1.Components[0], c1.Components[1], c1.Components[2]);
        glVertex2f(x1,y1);
        glVertex2f(x1,y0);
      }
      else // vertical
      {
        vtkm::Float32 x0 = x;
        vtkm::Float32 x1 = x + w;
        vtkm::Float32 y0 = y + h*v0;
        vtkm::Float32 y1 = y + h*v1;
        glColor3f(c0.Components[0], c0.Components[1], c0.Components[2]);
        glVertex2f(x0,y1);
        glVertex2f(x1,y1);
        glColor3f(c1.Components[0], c1.Components[1], c1.Components[2]);
        glVertex2f(x1,y0);
        glVertex2f(x0,y0);
      }
    }
    glEnd();
  }


  VTKM_CONT_EXPORT
  virtual void AddText(vtkm::Float32 x, vtkm::Float32 y,
                       vtkm::Float32 scale,
                       vtkm::Float32 angle,
                       vtkm::Float32 windowaspect,
                       vtkm::Float32 anchorx, vtkm::Float32 anchory,
                       Color color,
                       std::string text) const
  {
    glPushMatrix();
    glTranslatef(x,y,0);
    glScalef(1.f/windowaspect, 1, 1);
    glRotatef(angle, 0,0,1);
    glColor3f(color.Components[0], color.Components[1], color.Components[2]);
    this->RenderText(scale, anchorx, anchory, text);
    glPopMatrix();
  }

  VTKM_CONT_EXPORT
  virtual vtkm::rendering::WorldAnnotator *CreateWorldAnnotator() const
  {
    return new vtkm::rendering::WorldAnnotatorGL;
  }

private:
  BitmapFont Font;
  TextureGL FontTexture;

  void RenderText(vtkm::Float32 scale,
                  vtkm::Float32 anchorx, vtkm::Float32 anchory,
                  std::string text) const
  {
    if (this->FontTexture.ID == 0)
    {
      // When we load a font, we save a reference to it for the next time we
      // use it. Although technically we are changing the state, the logical
      // state does not change, so we go ahead and do it in this const
      // function.
      vtkm::rendering::CanvasGL *self =
          const_cast<vtkm::rendering::CanvasGL *>(this);
      self->Font = BitmapFontFactory::CreateLiberation2Sans();
      const std::vector<unsigned char> &rawpngdata =
          this->Font.GetRawImageData();

      std::vector<unsigned char> rgba;
      unsigned long width, height;
      int error = vtkm::rendering::DecodePNG(rgba, width, height,
                                             &rawpngdata[0], rawpngdata.size());
      if (error != 0)
      {
        return;
      }

      self->FontTexture.CreateAlphaFromRGBA(int(width),int(height),rgba);
    }


    this->FontTexture.Enable();

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_LIGHTING);
    //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -.5);

    glBegin(GL_QUADS);

    vtkm::Float32 textwidth = this->Font.GetTextWidth(text);

    vtkm::Float32 fx = -(.5f + .5f*anchorx) * textwidth;
    vtkm::Float32 fy = -(.5f + .5f*anchory);
    vtkm::Float32 fz = 0;
    for (unsigned int i=0; i<text.length(); ++i)
    {
      char c = text[i];
      char nextchar = (i < text.length()-1) ? text[i+1] : 0;

      vtkm::Float32 vl,vr,vt,vb;
      vtkm::Float32 tl,tr,tt,tb;
      this->Font.GetCharPolygon(c, fx, fy,
                          vl, vr, vt, vb,
                          tl, tr, tt, tb, nextchar);

      glTexCoord2f(tl, 1.f-tt);
      glVertex3f(scale*vl, scale*vt, fz);

      glTexCoord2f(tl, 1.f-tb);
      glVertex3f(scale*vl, scale*vb, fz);

      glTexCoord2f(tr, 1.f-tb);
      glVertex3f(scale*vr, scale*vb, fz);

      glTexCoord2f(tr, 1.f-tt);
      glVertex3f(scale*vr, scale*vt, fz);
    }

    glEnd();

    this->FontTexture.Disable();

    //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_ALPHA_TEST);
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasGL_h
