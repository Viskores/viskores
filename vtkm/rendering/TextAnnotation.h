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


}} //namespace vtkm::rendering
#endif //vtk_m_rendering_TextAnnotation_h
