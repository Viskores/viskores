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
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/RenderSurface.h>
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
  std::string   text;
  Color         color;
  vtkm::Float32 scale;
  vtkm::Float32 anchorx, anchory;

public:
  TextAnnotation(const std::string &txt, Color c, vtkm::Float32 s)
    : text(txt), color(c), scale(s)
  {
    // default anchor: bottom-left
    anchorx = -1;
    anchory = -1;
  }
  virtual ~TextAnnotation()
  {
  }
  void SetText(const std::string &txt)
  {
    text = txt;
  }
  void SetRawAnchor(vtkm::Float32 h, vtkm::Float32 v)
  {
    anchorx = h;
    anchory = v;
  }
  void SetAlignment(HorizontalAlignment h, VerticalAlignment v)
  {
    switch (h)
    {
      case Left:    anchorx = -1.0f; break;
      case HCenter: anchorx =  0.0f; break;
      case Right:   anchorx = +1.0f; break;
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
      case Bottom:  anchory = -1.0f;  break;
      case VCenter: anchory = -0.06f; break;
      case Top:     anchory = +1.0f;  break;
    }
  }
  void SetScale(vtkm::Float32 s)
  {
    scale = s;
  }
  virtual void Render(View &view,
                      WorldAnnotator &worldAnnotator,
                      RenderSurface &renderSurface) = 0;
};

class ScreenTextAnnotation : public TextAnnotation
{
protected:
  vtkm::Float32 x,y;
  vtkm::Float32 angle;
public:
  ScreenTextAnnotation(const std::string &txt, Color c, vtkm::Float32 s,
                       vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 angleDeg = 0.)
    : TextAnnotation(txt,c,s)
  {
    x = ox;
    y = oy;
    angle = angleDeg;
  }
  void SetPosition(vtkm::Float32 ox, vtkm::Float32 oy)
  {
    x = ox;
    y = oy;
  }
  virtual void Render(View &view,
                      WorldAnnotator &,
                      RenderSurface &renderSurface)
  {
    vtkm::Float32 WindowAspect = vtkm::Float32(view.Width) /
      vtkm::Float32(view.Height);

    //win->SetupForScreenSpace();
    renderSurface.AddText(x,y,
                          scale,
                          angle,
                          WindowAspect,
                          anchorx, anchory,
                          color, text);
  }
};

class BillboardTextAnnotation : public TextAnnotation
{
protected:
  vtkm::Float32 x,y,z;
  vtkm::Float32 angle;
public:
  BillboardTextAnnotation(const std::string &txt, Color c, vtkm::Float32 s,
                          vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 oz,
                          vtkm::Float32 angleDeg = 0.)
    : TextAnnotation(txt,c,s)
  {
    x = ox;
    y = oy;
    z = oz;
    angle = angleDeg;
  }
  void SetPosition(vtkm::Float32 ox, vtkm::Float32 oy, vtkm::Float32 oz)
  {
    x = ox;
    y = oy;
    z = oz;
  }

  virtual void Render(View &view,
                      WorldAnnotator &worldAnnotator,
                      RenderSurface &renderSurface)
  {
    vtkm::Matrix<vtkm::Float32, 4, 4> V, P;
    V = view.CreateViewMatrix();
    P = view.CreateProjectionMatrix();

    vtkm::Vec<vtkm::Float32,4> p4w(x,y,z,1);
    vtkm::Vec<vtkm::Float32,4> p4s = 
      vtkm::MatrixMultiply(vtkm::MatrixMultiply(P,V), p4w);

    renderSurface.SetViewToScreenSpace(view,true);

    vtkm::Float32 psx = p4s[0] / p4s[3];
    vtkm::Float32 psy = p4s[1] / p4s[3];
    vtkm::Float32 psz = p4s[2] / p4s[3];


    vtkm::Matrix<vtkm::Float32, 4, 4> T;
    T = MatrixHelpers::TranslateMatrix(psx,psy,-psz);

    vtkm::Float32 WindowAspect =
      vtkm::Float32(view.Width) / vtkm::Float32(view.Height);

    vtkm::Matrix<vtkm::Float32, 4, 4> SW;
    SW = MatrixHelpers::ScaleMatrix(1.f/WindowAspect, 1, 1);

    vtkm::Matrix<vtkm::Float32, 4, 4> SV;
    vtkm::MatrixIdentity(SV);
    //if view type == 2D?
    {
      vtkm::Float32 vl, vr, vb, vt;
      view.GetRealViewport(vl,vr,vb,vt);
      vtkm::Float32 xs = (vr-vl);
      vtkm::Float32 ys = (vt-vb);
      SV = MatrixHelpers::ScaleMatrix(2.f/xs, 2.f/ys, 1);
    }

    vtkm::Matrix<vtkm::Float32, 4, 4> R;
    R = MatrixHelpers::RotateZMatrix(angle * 3.14159265f / 180.f);

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
                           scale,
                           anchorx, anchory,
                           color, text);

    renderSurface.SetViewToWorldSpace(view,true);
  }
};


}} //namespace vtkm::rendering
#endif //vtk_m_rendering_TextAnnotation_h
