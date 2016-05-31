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
#ifndef vtk_m_rendering_RenderSurfaceGL_h
#define vtk_m_rendering_RenderSurfaceGL_h

#include <vtkm/Types.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/RenderSurface.h>
#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/TextureGL.h>
#include <vtkm/rendering/MatrixHelpers.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>

#include <iostream>
#include <fstream>

namespace vtkm {
namespace rendering {

class RenderSurfaceGL : public RenderSurface
{
public:
    VTKM_CONT_EXPORT
    RenderSurfaceGL(std::size_t w=1024, std::size_t h=1024,
          const vtkm::rendering::Color &c=vtkm::rendering::Color(0.0f,0.0f,0.0f,1.0f))
        : RenderSurface(w,h,c)
    {
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
        glFinish();
    }

    VTKM_CONT_EXPORT
    virtual void SetViewToWorldSpace(vtkm::rendering::View &v, bool clip)
    {
        vtkm::Float32 oglP[16], oglM[16];

        MatrixHelpers::CreateOGLMatrix(v.CreateProjectionMatrix(), oglP);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(oglP);
        MatrixHelpers::CreateOGLMatrix(v.CreateViewMatrix(), oglM);
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(oglM);

        SetViewportClipping(v, clip);
    }

    VTKM_CONT_EXPORT
    virtual void SetViewToScreenSpace(vtkm::rendering::View &v, bool clip)
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

        SetViewportClipping(v, clip);
    }

    VTKM_CONT_EXPORT
    virtual void SetViewportClipping(vtkm::rendering::View &v, bool clip)
    {
        if (clip)
        {
            vtkm::Float32 vl, vr, vb, vt;
            v.GetRealViewport(vl,vr,vb,vt);
            vtkm::Float32 _x = static_cast<vtkm::Float32>(v.Width)*(1.f+vl)/2.f;
            vtkm::Float32 _y = static_cast<vtkm::Float32>(v.Height)*(1.f+vb)/2.f;
            vtkm::Float32 _w = static_cast<vtkm::Float32>(v.Width)*(vr-vl)/2.f;
            vtkm::Float32 _h = static_cast<vtkm::Float32>(v.Height)*(vt-vb)/2.f;

            glViewport(static_cast<int>(_x), static_cast<int>(_y),
                       static_cast<int>(_w), static_cast<int>(_h));
        }
        else
        {
            glViewport(0,0, v.Width, v.Height);
        }
    }

    VTKM_CONT_EXPORT
    virtual void SaveAs(const std::string &fileName)
    {
        std::ofstream of(fileName.c_str());
        of << "P6" << std::endl
           << this->Width << " " << this->Height <<std::endl
           << 255 << std::endl;
        int height = static_cast<int>(this->Height);
        for (int yIndex=height-1; yIndex>=0; yIndex--)
            for (std::size_t xIndex=0; xIndex < this->Width; xIndex++)
            {
                const vtkm::Float32 *tuple =
                    &(this->ColorBuffer[static_cast<std::size_t>(yIndex)*this->Width*4 + xIndex*4]);
                of<<(unsigned char)(tuple[0]*255);
                of<<(unsigned char)(tuple[1]*255);
                of<<(unsigned char)(tuple[2]*255);
            }
        of.close();
    }

    VTKM_CONT_EXPORT
    virtual void AddLine(vtkm::Float64 x0, vtkm::Float64 y0,
                         vtkm::Float64 x1, vtkm::Float64 y1,
                         vtkm::Float32 linewidth,
                         const vtkm::rendering::Color &c)
    {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_LIGHTING);
        glColor3fv(c.Components);

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
                             bool horizontal)
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
                glColor3fv(c0.Components);
                glVertex2f(x0,y0);
                glVertex2f(x0,y1);
                glColor3fv(c1.Components);
                glVertex2f(x1,y1);
                glVertex2f(x1,y0);
            }
            else // vertical
            {
                vtkm::Float32 x0 = x;
                vtkm::Float32 x1 = x + w;
                vtkm::Float32 y0 = y + h*v0;
                vtkm::Float32 y1 = y + h*v1;
                glColor3fv(c0.Components);
                glVertex2f(x0,y1);
                glVertex2f(x1,y1);
                glColor3fv(c1.Components);
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
                       std::string text)
  {
    glPushMatrix();
    glTranslatef(x,y,0);
    glScalef(1.f/windowaspect, 1, 1);
    glRotatef(angle, 0,0,1);
    glColor3fv(color.Components);
    RenderText(scale, anchorx, anchory, text);
    glPopMatrix();
  }

private:
  BitmapFont font;
  TextureGL fontTexture;

  void RenderText(vtkm::Float32 scale,
                  vtkm::Float32 anchorx, vtkm::Float32 anchory,
                  std::string text)
  {
    if (fontTexture.id == 0)
    {
      font = BitmapFontFactory::CreateLiberation2Sans();
      std::vector<unsigned char> &rawpngdata = font.GetRawImageData();

      std::vector<unsigned char> rgba;
      unsigned long width, height;
      int error = decodePNG(rgba, width, height,
                            &rawpngdata[0], rawpngdata.size());
      if (error != 0)
      {
        return;
      }

      fontTexture.CreateAlphaFromRGBA(int(width),int(height),rgba);
    }


    fontTexture.Enable();

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_LIGHTING);
    //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -.5);

    glBegin(GL_QUADS);

    vtkm::Float32 textwidth = font.GetTextWidth(text);

    vtkm::Float32 fx = -(.5f + .5f*anchorx) * textwidth;
    vtkm::Float32 fy = -(.5f + .5f*anchory);
    vtkm::Float32 fz = 0;
    for (unsigned int i=0; i<text.length(); ++i)
    {
      char c = text[i];
      char nextchar = (i < text.length()-1) ? text[i+1] : 0;

      vtkm::Float32 vl,vr,vt,vb;
      vtkm::Float32 tl,tr,tt,tb;
      font.GetCharPolygon(c, fx, fy,
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

    fontTexture.Disable();

    //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0);
    glDepthMask(GL_TRUE);
    glDisable(GL_ALPHA_TEST);
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_RenderSurfaceGL_h
