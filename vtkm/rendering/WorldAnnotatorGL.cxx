//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/WorldAnnotatorGL.h>

#include <vtkm/Matrix.h>
#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/DecodePNG.h>
#include <vtkm/rendering/MatrixHelpers.h>
#include <vtkm/rendering/Scene.h>

#include <vtkm/rendering/internal/OpenGLHeaders.h>

namespace vtkm
{
namespace rendering
{

WorldAnnotatorGL::WorldAnnotatorGL(const vtkm::rendering::Canvas* canvas)
  : WorldAnnotator(canvas)
{
}

WorldAnnotatorGL::~WorldAnnotatorGL()
{
}

void WorldAnnotatorGL::AddLine(const vtkm::Vec3f_64& point0,
                               const vtkm::Vec3f_64& point1,
                               vtkm::Float32 lineWidth,
                               const vtkm::rendering::Color& color,
                               bool inFront) const
{
  if (inFront)
  {
    glDepthRange(-.0001, .9999);
  }

  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);

  glColor3f(color.Components[0], color.Components[1], color.Components[2]);

  glLineWidth(lineWidth);

  glBegin(GL_LINES);
  glVertex3d(point0[0], point0[1], point0[2]);
  glVertex3d(point1[0], point1[1], point1[2]);
  glEnd();

  if (inFront)
  {
    glDepthRange(0, 1);
  }
}

void WorldAnnotatorGL::AddText(const vtkm::Vec3f_32& origin,
                               const vtkm::Vec3f_32& right,
                               const vtkm::Vec3f_32& up,
                               vtkm::Float32 scale,
                               const vtkm::Vec2f_32& anchor,
                               const vtkm::rendering::Color& color,
                               const std::string& text,
                               const vtkm::Float32 vtkmNotUsed(depth)) const
{

  vtkm::Vec3f_32 n = vtkm::Cross(right, up);
  vtkm::Normalize(n);

  vtkm::Matrix<vtkm::Float32, 4, 4> m;
  m = MatrixHelpers::WorldMatrix(origin, right, up, n);

  vtkm::Float32 ogl[16];
  MatrixHelpers::CreateOGLMatrix(m, ogl);
  glPushMatrix();
  glMultMatrixf(ogl);
  glColor3f(color.Components[0], color.Components[1], color.Components[2]);
  this->RenderText(scale, anchor[0], anchor[1], text);
  glPopMatrix();
}

void WorldAnnotatorGL::RenderText(vtkm::Float32 scale,
                                  vtkm::Float32 anchorx,
                                  vtkm::Float32 anchory,
                                  std::string text) const
{
  if (!this->FontTexture.Valid())
  {
    // When we load a font, we save a reference to it for the next time we
    // use it. Although technically we are changing the state, the logical
    // state does not change, so we go ahead and do it in this const
    // function.
    vtkm::rendering::WorldAnnotatorGL* self = const_cast<vtkm::rendering::WorldAnnotatorGL*>(this);
    self->Font = BitmapFontFactory::CreateLiberation2Sans();
    const std::vector<unsigned char>& rawpngdata = this->Font.GetRawImageData();

    std::vector<unsigned char> rgba;
    unsigned long width, height;
    int error = vtkm::rendering::DecodePNG(rgba, width, height, &rawpngdata[0], rawpngdata.size());
    if (error != 0)
    {
      return;
    }

    self->FontTexture.CreateAlphaFromRGBA(int(width), int(height), rgba);
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

  vtkm::Float32 fx = -(.5f + .5f * anchorx) * textwidth;
  vtkm::Float32 fy = -(.5f + .5f * anchory);
  vtkm::Float32 fz = 0;
  for (unsigned int i = 0; i < text.length(); ++i)
  {
    char c = text[i];
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;

    vtkm::Float32 vl, vr, vt, vb;
    vtkm::Float32 tl, tr, tt, tb;
    this->Font.GetCharPolygon(c, fx, fy, vl, vr, vt, vb, tl, tr, tt, tb, nextchar);

    glTexCoord2f(tl, 1.f - tt);
    glVertex3f(scale * vl, scale * vt, fz);

    glTexCoord2f(tl, 1.f - tb);
    glVertex3f(scale * vl, scale * vb, fz);

    glTexCoord2f(tr, 1.f - tb);
    glVertex3f(scale * vr, scale * vb, fz);

    glTexCoord2f(tr, 1.f - tt);
    glVertex3f(scale * vr, scale * vt, fz);
  }

  glEnd();

  this->FontTexture.Disable();

  //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0);
  glDepthMask(GL_TRUE);
  glDisable(GL_ALPHA_TEST);
}
}
} // namespace vtkm::rendering
