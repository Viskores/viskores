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
#ifndef vtk_m_rendering_TextAnnotation_h
#define vtk_m_rendering_TextAnnotation_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/RenderSurface.h>
#include <vtkm/rendering/WorldAnnotator.h>
namespace vtkm {
namespace rendering {

class TextAnnotation
{
public:
  enum HorizontalAlignment
    {
      Left,
      HCenter,
      Right
    };
  enum VerticalAlignment
    {
      Bottom,
      VCenter,
      Top
    };

protected:
  std::string   Text;
  Color         TextColor;
  vtkm::Float32 Scale;
  vtkm::Float32 AnchorX, AnchorY;

public:
  TextAnnotation(const std::string &txt, Color c, vtkm::Float32 s)
    : Text(txt), TextColor(c), Scale(s)
  {
    // default anchor: bottom-left
    AnchorX = -1;
    AnchorY = -1;
  }
  virtual ~TextAnnotation()
  {
  }
  void SetText(const std::string &txt)
  {
    Text = txt;
  }
  void SetRawAnchor(vtkm::Float32 h, vtkm::Float32 v)
  {
    AnchorX = h;
    AnchorY = v;
  }
  void SetAlignment(HorizontalAlignment h, VerticalAlignment v)
  {
    switch (h)
    {
      case Left:    AnchorX = -1.0f; break;
      case HCenter: AnchorX =  0.0f; break;
      case Right:   AnchorX = +1.0f; break;
    }

    // For vertical alignment, "center" is generally the center
    // of only the above-baseline contents of the font, so we
    // use a value slightly off of zero for VCenter.
    // (We don't use an offset value instead of -1.0 for the
    // bottom value, because generally we want a true minimum
    // extent, e.g. to have text sitting at the bottom of a
    // window, and in that case, we need to keep all the text,
    // including parts that descend below the baseline, above
    // the bottom of the window.
    switch (v)
    {
      case Bottom:  AnchorY = -1.0f;  break;
      case VCenter: AnchorY = -0.06f; break;
      case Top:     AnchorY = +1.0f;  break;
    }
  }
  void SetScale(vtkm::Float32 s)
  {
    Scale = s;
  }
  virtual void Render(Camera &camera,
                      WorldAnnotator &worldAnnotator,
                      RenderSurface &renderSurface) = 0;
};

class ScreenTextAnnotation : public TextAnnotation
{
protected:
  vtkm::Float32 XPos,YPos;
  vtkm::Float32 Angle;
public:
  ScreenTextAnnotation(const std::string &txt, Color c, vtkm::Float32 s,
                       vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 angleDeg = 0.)
    : TextAnnotation(txt,c,s)
  {
    XPos = ox;
    YPos = oy;
    Angle = angleDeg;
  }
  void SetPosition(vtkm::Float32 ox, vtkm::Float32 oy)
  {
    XPos = ox;
    YPos = oy;
  }
  virtual void Render(Camera &camera,
                      WorldAnnotator &,
                      RenderSurface &renderSurface)
  {
    vtkm::Float32 WindowAspect = vtkm::Float32(camera.Width) /
      vtkm::Float32(camera.Height);

    renderSurface.AddText(XPos,YPos,
                          Scale,
                          Angle,
                          WindowAspect,
                          AnchorX, AnchorY,
                          TextColor, Text);
  }
};

class BillboardTextAnnotation : public TextAnnotation
{
protected:
  vtkm::Float32 XPos,YPos,ZPos;
  vtkm::Float32 Angle;
public:
  BillboardTextAnnotation(const std::string &txt, Color c, vtkm::Float32 s,
                          vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 oz,
                          vtkm::Float32 angleDeg = 0.)
    : TextAnnotation(txt,c,s)
  {
    XPos = ox;
    YPos = oy;
    ZPos = oz;
    Angle = angleDeg;
  }
  void SetPosition(vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 oz)
  {
    XPos = ox;
    YPos = oy;
    ZPos = oz;
  }

  virtual void Render(Camera &camera,
                      WorldAnnotator &worldAnnotator,
                      RenderSurface &renderSurface)
  {
    vtkm::Matrix<vtkm::Float32, 4, 4> V, P;
    V = camera.CreateViewMatrix();
    P = camera.CreateProjectionMatrix();

    vtkm::Vec<vtkm::Float32,4> p4w(XPos,YPos,ZPos,1);
    vtkm::Vec<vtkm::Float32,4> p4s =
      vtkm::MatrixMultiply(vtkm::MatrixMultiply(P,V), p4w);

    renderSurface.SetViewToScreenSpace(camera,true);

    vtkm::Float32 psx = p4s[0] / p4s[3];
    vtkm::Float32 psy = p4s[1] / p4s[3];
    vtkm::Float32 psz = p4s[2] / p4s[3];


    vtkm::Matrix<vtkm::Float32, 4, 4> T;
    T = MatrixHelpers::TranslateMatrix(psx,psy,-psz);

    vtkm::Float32 WindowAspect =
      vtkm::Float32(camera.Width) / vtkm::Float32(camera.Height);

    vtkm::Matrix<vtkm::Float32, 4, 4> SW;
    SW = MatrixHelpers::ScaleMatrix(1.f/WindowAspect, 1, 1);

    vtkm::Matrix<vtkm::Float32, 4, 4> SV;
    vtkm::MatrixIdentity(SV);
    //if view type == 2D?
    {
      vtkm::Float32 vl, vr, vb, vt;
      camera.GetRealViewport(vl,vr,vb,vt);
      vtkm::Float32 xs = (vr-vl);
      vtkm::Float32 ys = (vt-vb);
      SV = MatrixHelpers::ScaleMatrix(2.f/xs, 2.f/ys, 1);
    }

    vtkm::Matrix<vtkm::Float32, 4, 4> R;
    R = MatrixHelpers::RotateZMatrix(Angle * 3.14159265f / 180.f);

    vtkm::Vec<vtkm::Float32,4> origin4(0,0,0,1);
    vtkm::Vec<vtkm::Float32,4> right4(1,0,0,0);
    vtkm::Vec<vtkm::Float32,4> up4(0,1,0,0);

    vtkm::Matrix<vtkm::Float32, 4, 4> M =
      vtkm::MatrixMultiply(T,
      vtkm::MatrixMultiply(SW,
      vtkm::MatrixMultiply(SV,
                           R)));

    vtkm::Vec<vtkm::Float32,4> new_origin4 =
      vtkm::MatrixMultiply(M, origin4);
    vtkm::Vec<vtkm::Float32,4> new_right4 =
      vtkm::MatrixMultiply(M, right4);
    vtkm::Vec<vtkm::Float32,4> new_up4 =
      vtkm::MatrixMultiply(M, up4);

    vtkm::Float32 px = new_origin4[0] / new_origin4[3];
    vtkm::Float32 py = new_origin4[1] / new_origin4[3];
    vtkm::Float32 pz = new_origin4[2] / new_origin4[3];

    vtkm::Float32 rx = new_right4[0];
    vtkm::Float32 ry = new_right4[1];
    vtkm::Float32 rz = new_right4[2];

    vtkm::Float32 ux = new_up4[0];
    vtkm::Float32 uy = new_up4[1];
    vtkm::Float32 uz = new_up4[2];

    worldAnnotator.AddText(px,py,pz,
                           rx,ry,rz,
                           ux,uy,uz,
                           Scale,
                           AnchorX, AnchorY,
                           TextColor, Text);

    renderSurface.SetViewToWorldSpace(camera,true);
  }
};


}} //namespace vtkm::rendering
#endif //vtk_m_rendering_TextAnnotation_h
