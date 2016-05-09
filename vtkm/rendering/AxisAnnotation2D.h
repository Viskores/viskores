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
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/RenderSurface.h>
#include <vtkm/rendering/WorldAnnotator.h>
#include <vtkm/rendering/AxisAnnotation.h>

namespace vtkm {
namespace rendering {

class AxisAnnotation2D : public AxisAnnotation
{
protected:
  double maj_tx, maj_ty, maj_toff;
  double min_tx, min_ty, min_toff;
  double x0, y0, x1, y1;
  double lower, upper;
  double fontscale;
  float  linewidth;
  Color  color;
  bool   logarithmic;
#if 0
  eavlTextAnnotation::HorizontalAlignment halign;
  eavlTextAnnotation::VerticalAlignment valign;
  std::vector<eavlTextAnnotation*> labels;
#endif

  std::vector<double> maj_positions;
  std::vector<double> maj_proportions;

  std::vector<double> min_positions;
  std::vector<double> min_proportions;

  ///\todo: Don't need anymore??
  bool worldSpace;

  int moreOrLessTickAdjustment;
public:
  AxisAnnotation2D() : AxisAnnotation()
  {
#if 0
    halign = eavlTextAnnotation::HCenter;
    valign = eavlTextAnnotation::VCenter;
#endif
    fontscale = 0.05;
    linewidth = 1.0;
    color = Color(1,1,1);
    logarithmic = false;
    moreOrLessTickAdjustment = 0;
    worldSpace = false;
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
  void SetWorldSpace(bool ws)
  {
    worldSpace = ws;
  }
  void SetMoreOrLessTickAdjustment(int offset)
  {
    moreOrLessTickAdjustment = offset;
  }
  void SetColor(Color c)
  {
    color = c;
  }
  void SetLineWidth(float lw)
  {
    linewidth = lw;
  }
  void SetMajorTickSize(double xlen, double ylen, double offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    maj_tx=xlen;
    maj_ty=ylen;
    maj_toff = offset;
  }
  void SetMinorTickSize(double xlen, double ylen, double offset)
  {
    min_tx=xlen;
    min_ty=ylen;
    min_toff = offset;
  }
  ///\todo: rename, since it might be screen OR world position?
  void SetScreenPosition(double x0_, double y0_,
                         double x1_, double y1_)
  {
    x0 = x0_;
    y0 = y0_;

    x1 = x1_;
    y1 = y1_;
  }
#if 0
  void SetLabelAlignment(eavlTextAnnotation::HorizontalAlignment h,
                         eavlTextAnnotation::VerticalAlignment v)
  {
    halign = h;
    valign = v;
  }
#endif
  void SetLabelFontScale(double s)
  {
    fontscale = s;
#if 0
    for (unsigned int i=0; i<labels.size(); i++)
      labels[i]->SetScale(s);
#endif
  }
  void SetRangeForAutoTicks(double l, double u)
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
  void SetMajorTicks(const std::vector<double> &pos, const std::vector<double> &prop)
  {
    maj_positions.clear();
    maj_positions.insert(maj_positions.begin(), pos.begin(), pos.end());

    maj_proportions.clear();
    maj_proportions.insert(maj_proportions.begin(), prop.begin(), prop.end());
  }
  void SetMinorTicks(const std::vector<double> &pos, const std::vector<double> &prop)
  {
    min_positions.clear();
    min_positions.insert(min_positions.begin(), pos.begin(), pos.end());

    min_proportions.clear();
    min_proportions.insert(min_proportions.begin(), prop.begin(), prop.end());
  }
  virtual void Render(View &,
                      WorldAnnotator &,
                      RenderSurface &renderSurface)
  {
    renderSurface.AddLine(x0,y0, x1,y1, linewidth, color);

    // major ticks
    unsigned int nmajor = (unsigned int)maj_proportions.size();
#if 0
    while (labels.size() < nmajor)
    {
      if (worldSpace)
      {
        labels.push_back(new eavlBillboardTextAnnotation(win,"test",
                                                         color,
                                                         fontscale,
                                                         0,0,0, true));
      }
      else
      {
        labels.push_back(new eavlScreenTextAnnotation(win,"test",
                                                      color,
                                                      fontscale,
                                                      0,0, 0));
      }
    }
#endif
    for (unsigned int i=0; i<nmajor; ++i)
    {
      double xc = x0 + (x1-x0) * maj_proportions[i];
      double yc = y0 + (y1-y0) * maj_proportions[i];
      double xs = xc - maj_tx*maj_toff;
      double xe = xc + maj_tx*(1. - maj_toff);
      double ys = yc - maj_ty*maj_toff;
      double ye = yc + maj_ty*(1. - maj_toff);

      renderSurface.AddLine(xs,ys, xe,ye, 1.0, color);

      if (maj_ty == 0)
      {
        // slight shift to space between label and tick
        xs -= (maj_tx<0?-1.:+1.) * fontscale * .1;
      }

      char val[256];
      snprintf(val, 256, "%g", maj_positions[i]);

#if 0
      labels[i]->SetText(val);
      //if (fabs(maj_positions[i]) < 1e-10)
      //    labels[i]->SetText("0");
      if (worldSpace)
        ((eavlBillboardTextAnnotation*)(labels[i]))->SetPosition(xs,ys,0);
      else
        ((eavlScreenTextAnnotation*)(labels[i]))->SetPosition(xs,ys);

      labels[i]->SetAlignment(halign,valign);
#endif
    }

    // minor ticks
    if (min_tx != 0 || min_ty != 0)
    {
      unsigned int nminor = (unsigned int)min_proportions.size();
      for (unsigned int i=0; i<nminor; ++i)
      {
        double xc = x0 + (x1-x0) * min_proportions[i];
        double yc = y0 + (y1-y0) * min_proportions[i];
        double xs = xc - min_tx*min_toff;
        double xe = xc + min_tx*(1. - min_toff);
        double ys = yc - min_ty*min_toff;
        double ye = yc + min_ty*(1. - min_toff);

        renderSurface.AddLine(xs,ys, xe,ye, 1.0, color);
      }
    }

#if 0
    for (int i=0; i<nmajor; ++i)
    {
      labels[i]->Render(view);
    }
#endif

  }
};

}} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation2D_h
