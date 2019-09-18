//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/CanvasGL.h>

#include <vtkm/rendering/BitmapFontFactory.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/DecodePNG.h>
#include <vtkm/rendering/MatrixHelpers.h>
#include <vtkm/rendering/WorldAnnotatorGL.h>
#include <vtkm/rendering/internal/OpenGLHeaders.h>

#include <vtkm/cont/ColorTable.hxx>

namespace vtkm
{
namespace rendering
{

CanvasGL::CanvasGL(vtkm::Id width, vtkm::Id height)
  : Canvas(width, height)
{
}

CanvasGL::~CanvasGL()
{
}

void CanvasGL::Initialize()
{
  // Nothing to initialize
}

void CanvasGL::Activate()
{
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  this->ResizeBuffers(viewport[2], viewport[3]);

  glEnable(GL_DEPTH_TEST);
}

void CanvasGL::Clear()
{
  vtkm::rendering::Color backgroundColor = this->GetBackgroundColor();
  glClearColor(backgroundColor.Components[0],
               backgroundColor.Components[1],
               backgroundColor.Components[2],
               1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CanvasGL::Finish()
{
  glFinish();
}

vtkm::rendering::Canvas* CanvasGL::NewCopy() const
{
  return new vtkm::rendering::CanvasGL(*this);
}

void CanvasGL::SetViewToWorldSpace(const vtkm::rendering::Camera& camera, bool clip)
{
  vtkm::Float32 oglP[16], oglM[16];

  MatrixHelpers::CreateOGLMatrix(camera.CreateProjectionMatrix(this->GetWidth(), this->GetHeight()),
                                 oglP);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(oglP);
  MatrixHelpers::CreateOGLMatrix(camera.CreateViewMatrix(), oglM);
  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(oglM);

  this->SetViewportClipping(camera, clip);
}

void CanvasGL::SetViewToScreenSpace(const vtkm::rendering::Camera& camera, bool clip)
{
  vtkm::Float32 oglP[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  vtkm::Float32 oglM[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  oglP[0 * 4 + 0] = 1.;
  oglP[1 * 4 + 1] = 1.;
  oglP[2 * 4 + 2] = -1.;
  oglP[3 * 4 + 3] = 1.;

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(oglP);

  oglM[0 * 4 + 0] = 1.;
  oglM[1 * 4 + 1] = 1.;
  oglM[2 * 4 + 2] = 1.;
  oglM[3 * 4 + 3] = 1.;

  glMatrixMode(GL_MODELVIEW);
  glLoadMatrixf(oglM);

  this->SetViewportClipping(camera, clip);
}

void CanvasGL::SetViewportClipping(const vtkm::rendering::Camera& camera, bool clip)
{
  if (clip)
  {
    vtkm::Float32 vl, vr, vb, vt;
    camera.GetRealViewport(this->GetWidth(), this->GetHeight(), vl, vr, vb, vt);
    vtkm::Float32 _x = static_cast<vtkm::Float32>(this->GetWidth()) * (1.f + vl) / 2.f;
    vtkm::Float32 _y = static_cast<vtkm::Float32>(this->GetHeight()) * (1.f + vb) / 2.f;
    vtkm::Float32 _w = static_cast<vtkm::Float32>(this->GetWidth()) * (vr - vl) / 2.f;
    vtkm::Float32 _h = static_cast<vtkm::Float32>(this->GetHeight()) * (vt - vb) / 2.f;

    glViewport(static_cast<GLint>(_x),
               static_cast<GLint>(_y),
               static_cast<GLsizei>(_w),
               static_cast<GLsizei>(_h));
  }
  else
  {
    glViewport(
      0, 0, static_cast<GLsizei>(this->GetWidth()), static_cast<GLsizei>(this->GetHeight()));
  }
}

void CanvasGL::RefreshColorBuffer() const
{
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  VTKM_ASSERT(viewport[2] == this->GetWidth());
  VTKM_ASSERT(viewport[3] == this->GetHeight());

  glReadPixels(viewport[0],
               viewport[1],
               viewport[2],
               viewport[3],
               GL_RGBA,
               GL_FLOAT,
               const_cast<vtkm::Vec<float, 4>*>(this->GetColorBuffer().GetStorage().GetArray()));
}

void CanvasGL::RefreshDepthBuffer() const
{
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  VTKM_ASSERT(viewport[2] == this->GetWidth());
  VTKM_ASSERT(viewport[3] == this->GetHeight());

  glReadPixels(viewport[0],
               viewport[1],
               viewport[2],
               viewport[3],
               GL_DEPTH_COMPONENT,
               GL_FLOAT,
               const_cast<vtkm::Float32*>(this->GetDepthBuffer().GetStorage().GetArray()));
}

void CanvasGL::AddColorSwatch(const vtkm::Vec2f_64& point0,
                              const vtkm::Vec2f_64& point1,
                              const vtkm::Vec2f_64& point2,
                              const vtkm::Vec2f_64& point3,
                              const vtkm::rendering::Color& color) const
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);

  glBegin(GL_QUADS);
  glColor3f(color.Components[0], color.Components[1], color.Components[2]);
  glTexCoord1f(0);
  glVertex3f(float(point0[0]), float(point0[1]), .99f);
  glVertex3f(float(point1[0]), float(point1[1]), .99f);

  glTexCoord1f(1);
  glVertex3f(float(point2[0]), float(point2[1]), .99f);
  glVertex3f(float(point3[0]), float(point3[1]), .99f);
  glEnd();
}

void CanvasGL::AddLine(const vtkm::Vec2f_64& point0,
                       const vtkm::Vec2f_64& point1,
                       vtkm::Float32 linewidth,
                       const vtkm::rendering::Color& color) const
{
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glColor3f(color.Components[0], color.Components[1], color.Components[2]);

  glLineWidth(linewidth);

  glBegin(GL_LINES);
  glVertex2f(float(point0[0]), float(point0[1]));
  glVertex2f(float(point1[0]), float(point1[1]));
  glEnd();
}

void CanvasGL::AddColorBar(const vtkm::Bounds& bounds,
                           const vtkm::cont::ColorTable& colorTable,
                           bool horizontal) const
{
  const int n = 256;
  //map through the color table for our 256 + 1 points as the first step
  vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
  colorTable.Sample(n + 1, colors);

  vtkm::Float32 startX = static_cast<vtkm::Float32>(bounds.X.Min);
  vtkm::Float32 startY = static_cast<vtkm::Float32>(bounds.Y.Min);
  vtkm::Float32 width = static_cast<vtkm::Float32>(bounds.X.Length());
  vtkm::Float32 height = static_cast<vtkm::Float32>(bounds.Y.Length());
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glBegin(GL_QUADS);

  auto colorPortal = colors.GetPortalConstControl();

  for (int i = 0; i < n; i++)
  {
    vtkm::Float32 v0 = static_cast<vtkm::Float32>(i) / static_cast<vtkm::Float32>(n);
    vtkm::Float32 v1 = static_cast<vtkm::Float32>(i + 1) / static_cast<vtkm::Float32>(n);
    auto c0 = colorPortal.Get(i);
    auto c1 = colorPortal.Get(i + 1);
    if (horizontal)
    {
      vtkm::Float32 x0 = startX + width * v0;
      vtkm::Float32 x1 = startX + width * v1;
      vtkm::Float32 y0 = startY;
      vtkm::Float32 y1 = startY + height;
      glColor3ub(c0[0], c0[1], c0[2]);
      glVertex2f(x0, y0);
      glVertex2f(x0, y1);
      glColor3ub(c1[0], c1[1], c1[2]);
      glVertex2f(x1, y1);
      glVertex2f(x1, y0);
    }
    else // vertical
    {
      vtkm::Float32 x0 = startX;
      vtkm::Float32 x1 = startX + width;
      vtkm::Float32 y0 = startY + height * v0;
      vtkm::Float32 y1 = startY + height * v1;
      glColor3ub(c0[0], c0[1], c0[2]);
      glVertex2f(x0, y1);
      glVertex2f(x1, y1);
      glColor3ub(c1[0], c1[1], c1[2]);
      glVertex2f(x1, y0);
      glVertex2f(x0, y0);
    }
  }
  glEnd();
}

void CanvasGL::AddText(const vtkm::Vec2f_32& position,
                       vtkm::Float32 scale,
                       vtkm::Float32 angle,
                       vtkm::Float32 windowAspect,
                       const vtkm::Vec2f_32& anchor,
                       const vtkm::rendering::Color& color,
                       const std::string& text) const
{
  glPushMatrix();
  glTranslatef(position[0], position[1], 0);
  glScalef(1.f / windowAspect, 1, 1);
  glRotatef(angle, 0, 0, 1);
  glColor3f(color.Components[0], color.Components[1], color.Components[2]);
  this->RenderText(scale, anchor, text);
  glPopMatrix();
}

vtkm::rendering::WorldAnnotator* CanvasGL::CreateWorldAnnotator() const
{
  return new vtkm::rendering::WorldAnnotatorGL(this);
}

void CanvasGL::RenderText(vtkm::Float32 scale,
                          const vtkm::Vec2f_32& anchor,
                          const std::string& text) const
{
  if (!this->FontTexture.Valid())
  {
    // When we load a font, we save a reference to it for the next time we
    // use it. Although technically we are changing the state, the logical
    // state does not change, so we go ahead and do it in this const
    // function.
    vtkm::rendering::CanvasGL* self = const_cast<vtkm::rendering::CanvasGL*>(this);
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

  vtkm::Float32 fx = -(.5f + .5f * anchor[0]) * textwidth;
  vtkm::Float32 fy = -(.5f + .5f * anchor[1]);
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
