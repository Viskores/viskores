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
#ifndef vtk_m_rendering_AxisAnnotation2D_h
#define vtk_m_rendering_AxisAnnotation2D_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/RenderSurface.h>
#include <vtkm/rendering/WorldAnnotator.h>
#include <vtkm/rendering/AxisAnnotation.h>
#include <vtkm/rendering/TextAnnotation.h>

namespace vtkm {
namespace rendering {

class AxisAnnotation2D : public AxisAnnotation
{
protected:
  vtkm::Float64 maj_tx, maj_ty, maj_toff;
  vtkm::Float64 min_tx, min_ty, min_toff;
  vtkm::Float64 x0, y0, x1, y1;
  vtkm::Float64 lower, upper;
  vtkm::Float32 fontscale;
  vtkm::Float32  linewidth;
  vtkm::rendering::Color  color;
  bool   logarithmic;

  TextAnnotation::HorizontalAlignment halign;
  TextAnnotation::VerticalAlignment valign;
  std::vector<TextAnnotation*> labels;


  std::vector<vtkm::Float64> maj_positions;
  std::vector<vtkm::Float64> maj_proportions;

  std::vector<vtkm::Float64> min_positions;
  std::vector<vtkm::Float64> min_proportions;

  int moreOrLessTickAdjustment;
public:
  AxisAnnotation2D() : AxisAnnotation()
  {
    halign = TextAnnotation::HCenter;
    valign = TextAnnotation::VCenter;
    fontscale = 0.05f;
    linewidth = 1.0;
    color = Color(1,1,1);
    logarithmic = false;
    moreOrLessTickAdjustment = 0;
  }
  virtual ~AxisAnnotation2D()
  {
  }
#if 0
  void SetLogarithmic(bool l)
  {
    logarithmic = l;
  }
#endif
  void SetMoreOrLessTickAdjustment(int offset)
  {
    moreOrLessTickAdjustment = offset;
  }
  void SetColor(vtkm::rendering::Color c)
  {
    color = c;
  }
  void SetLineWidth(vtkm::Float32 lw)
  {
    linewidth = lw;
  }
  void SetMajorTickSize(vtkm::Float64 xlen, vtkm::Float64 ylen, vtkm::Float64 offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    maj_tx=xlen;
    maj_ty=ylen;
    maj_toff = offset;
  }
  void SetMinorTickSize(vtkm::Float64 xlen, vtkm::Float64 ylen, vtkm::Float64 offset)
  {
    min_tx=xlen;
    min_ty=ylen;
    min_toff = offset;
  }
  ///\todo: rename, since it might be screen OR world position?
  void SetScreenPosition(vtkm::Float64 x0_, vtkm::Float64 y0_,
                         vtkm::Float64 x1_, vtkm::Float64 y1_)
  {
    x0 = x0_;
    y0 = y0_;

    x1 = x1_;
    y1 = y1_;
  }
  void SetLabelAlignment(TextAnnotation::HorizontalAlignment h,
                         TextAnnotation::VerticalAlignment v)
  {
    halign = h;
    valign = v;
  }
  void SetLabelFontScale(vtkm::Float32 s)
  {
    fontscale = s;
    for (unsigned int i=0; i<labels.size(); i++)
      labels[i]->SetScale(s);
  }
  void SetRangeForAutoTicks(vtkm::Float64 l, vtkm::Float64 u)
  {
    lower = l;
    upper = u;

#if 0
    if (logarithmic)
    {
      CalculateTicksLogarithmic(lower, upper, false, maj_positions, maj_proportions, moreOrLessTickAdjustment);
      CalculateTicksLogarithmic(lower, upper, true,  min_positions, min_proportions, moreOrLessTickAdjustment);
    }
    else
#endif
    {
      CalculateTicks(lower, upper, false, maj_positions, maj_proportions, moreOrLessTickAdjustment);
      CalculateTicks(lower, upper, true,  min_positions, min_proportions, moreOrLessTickAdjustment);
    }
  }
  void SetMajorTicks(const std::vector<vtkm::Float64> &pos, const std::vector<vtkm::Float64> &prop)
  {
    maj_positions.clear();
    maj_positions.insert(maj_positions.begin(), pos.begin(), pos.end());

    maj_proportions.clear();
    maj_proportions.insert(maj_proportions.begin(), prop.begin(), prop.end());
  }
  void SetMinorTicks(const std::vector<vtkm::Float64> &pos, const std::vector<vtkm::Float64> &prop)
  {
    min_positions.clear();
    min_positions.insert(min_positions.begin(), pos.begin(), pos.end());

    min_proportions.clear();
    min_proportions.insert(min_proportions.begin(), prop.begin(), prop.end());
  }
  virtual void Render(Camera &camera,
                      WorldAnnotator &worldAnnotator,
                      RenderSurface &renderSurface)
  {
    renderSurface.AddLine(x0,y0, x1,y1, linewidth, color);

    // major ticks
    unsigned int nmajor = (unsigned int)maj_proportions.size();
    while (labels.size() < nmajor)
    {
        labels.push_back(new ScreenTextAnnotation("test",
                                                  color,
                                                  fontscale,
                                                  0,0, 0));
    }

    for (unsigned int i=0; i<nmajor; ++i)
    {
      vtkm::Float64 xc = x0 + (x1-x0) * maj_proportions[i];
      vtkm::Float64 yc = y0 + (y1-y0) * maj_proportions[i];
      vtkm::Float64 xs = xc - maj_tx*maj_toff;
      vtkm::Float64 xe = xc + maj_tx*(1. - maj_toff);
      vtkm::Float64 ys = yc - maj_ty*maj_toff;
      vtkm::Float64 ye = yc + maj_ty*(1. - maj_toff);

      renderSurface.AddLine(xs,ys, xe,ye, 1.0, color);

      if (maj_ty == 0)
      {
        // slight shift to space between label and tick
        xs -= (maj_tx<0?-1.:+1.) * fontscale * .1;
      }

      char val[256];
      snprintf(val, 256, "%g", maj_positions[i]);

      labels[i]->SetText(val);
      //if (fabs(maj_positions[i]) < 1e-10)
      //    labels[i]->SetText("0");
      ((ScreenTextAnnotation*)(labels[i]))->SetPosition(vtkm::Float32(xs),
                                                        vtkm::Float32(ys));

      labels[i]->SetAlignment(halign,valign);
    }

    // minor ticks
    if (min_tx != 0 || min_ty != 0)
    {
      unsigned int nminor = (unsigned int)min_proportions.size();
      for (unsigned int i=0; i<nminor; ++i)
      {
        vtkm::Float64 xc = x0 + (x1-x0) * min_proportions[i];
        vtkm::Float64 yc = y0 + (y1-y0) * min_proportions[i];
        vtkm::Float64 xs = xc - min_tx*min_toff;
        vtkm::Float64 xe = xc + min_tx*(1. - min_toff);
        vtkm::Float64 ys = yc - min_ty*min_toff;
        vtkm::Float64 ye = yc + min_ty*(1. - min_toff);

        renderSurface.AddLine(xs,ys, xe,ye, 1.0, color);
      }
    }

    for (unsigned int i=0; i<nmajor; ++i)
    {
      labels[i]->Render(camera, worldAnnotator, renderSurface);
    }
  }
};

}} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation2D_h
