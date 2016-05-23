//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_WorldAnnotatorGL_h
#define vtk_m_rendering_WorldAnnotatorGL_h

#include <vtkm/Matrix.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>
#include <vtkm/rendering/BitmapFont.h>
#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/TextureGL.h>
#include <vtkm/rendering/MatrixHelpers.h>

namespace vtkm {
namespace rendering {

class WorldAnnotatorGL : public WorldAnnotator
{
public:
  virtual void AddLine(vtkm::Float64 x0, vtkm::Float64 y0, vtkm::Float64 z0,
                       vtkm::Float64 x1, vtkm::Float64 y1, vtkm::Float64 z1,
                       vtkm::Float32 linewidth,
                       const vtkm::rendering::Color &c,
                       bool infront)
  {
    if (infront)
      glDepthRange(-.0001,.9999);

    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glColor3fv(c.Components);

    glLineWidth(linewidth);

    glBegin(GL_LINES);
    glVertex3d(x0,y0,z0);
    glVertex3d(x1,y1,z1);
    glEnd();

    if (infront)
      glDepthRange(0,1);

  }
  virtual void AddText(vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 oz,
                       vtkm::Float32 rx, vtkm::Float32 ry, vtkm::Float32 rz,
                       vtkm::Float32 ux, vtkm::Float32 uy, vtkm::Float32 uz,
                       vtkm::Float32 scale,
                       vtkm::Float32 anchorx, vtkm::Float32 anchory,
                       Color color,
                       std::string text)
  {
    vtkm::Vec<vtkm::Float32,3> o(ox,oy,oz);
    vtkm::Vec<vtkm::Float32,3> r(rx,ry,rz);
    vtkm::Vec<vtkm::Float32,3> u(ux,uy,uz);

    vtkm::Vec<vtkm::Float32,3> n = vtkm::Cross(r,u);
    vtkm::Normalize(n);

    vtkm::Matrix<vtkm::Float32,4,4> m;
    m = MatrixHelpers::WorldMatrix(o, r, u, n);

    vtkm::Float32 ogl[16];
    MatrixHelpers::CreateOGLMatrix(m, ogl);
    glPushMatrix();
    glMultMatrixf(ogl);
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

#endif // vtk_m_rendering_WorldAnnotatorGL_h
