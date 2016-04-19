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
#ifndef vtk_m_rendering_AxisAnnotation3D_h
#define vtk_m_rendering_AxisAnnotation3D_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>
#include <vtkm/rendering/AxisAnnotation.h>

namespace vtkm {
namespace rendering {

class AxisAnnotation3D : public AxisAnnotation
{
private:
protected:
  double maj_size, maj_toff;
  double min_size, min_toff;
  int    axis;
  double invertx, inverty, invertz;
  double x0, y0, z0,   x1, y1, z1;
  double lower, upper;
  double fontscale;
  Color color;
  //vector<BillboardTextAnnotation*> labels; ///<\todo: add text back in 
  int moreOrLessTickAdjustment;
public:
  AxisAnnotation3D() : AxisAnnotation()
  {
    axis = 0;
    color = Color(1,1,1);
  }
  virtual ~AxisAnnotation3D()
  {
  }
  void SetMoreOrLessTickAdjustment(int offset)
  {
    moreOrLessTickAdjustment = offset;
  }
  void SetColor(Color c)
  {
    color = c;
  }
  void SetAxis(int a)
  {
    axis = a;
  }
  void SetTickInvert(bool x, bool y, bool z)
  {
    invertx = x ? +1 : -1;
    inverty = y ? +1 : -1;
    invertz = z ? +1 : -1;
  }
  void SetMajorTickSize(double size, double offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    maj_size = size;
    maj_toff = offset;
  }
  void SetMinorTickSize(double size, double offset)
  {
    min_size = size;
    min_toff = offset;
  }
  void SetWorldPosition(double x0_, double y0_, double z0_,
                        double x1_, double y1_, double z1_)
  {
    x0 = x0_;
    y0 = y0_;
    z0 = z0_;

    x1 = x1_;
    y1 = y1_;
    z1 = z1_;
  }
  void SetLabelFontScale(float s)
  {
    fontscale = s;
#if 0
    for (unsigned int i=0; i<labels.size(); i++)
      labels[i]->SetScale(s);
#endif
  }
  void SetRange(double l, double u)
  {
    lower = l;
    upper = u;
  }
  virtual void Render(View &view,
                      WorldAnnotator &worldannotator)
  {
    float linewidth = 1.0;
    bool infront = true;
    worldannotator.AddLine(x0,y0,z0,
                            x1,y1,z1,
                            linewidth, color, infront);

    std::vector<double> positions;
    std::vector<double> proportions;
    // major ticks
    CalculateTicks(lower, upper, false, positions, proportions, moreOrLessTickAdjustment);
    unsigned int nmajor = (unsigned int)proportions.size();
#if 0
    while ((int)labels.size() < nmajor)
    {
      labels.push_back(new BillboardTextAnnotation(win,"test",
                                                       color,
                                                       fontscale,
                                                       0,0,0, false, 0));
    }
#endif
    for (unsigned int i=0; i<nmajor; ++i)
    {
      double xc = x0 + (x1-x0) * proportions[i];
      double yc = y0 + (y1-y0) * proportions[i];
      double zc = z0 + (z1-z0) * proportions[i];
      for (int pass=0; pass<=1; pass++)
      {
        double tx=0, ty=0, tz=0;
        switch (axis)
        {
          case 0: if (pass==0) ty=maj_size; else tz=maj_size; break;
          case 1: if (pass==0) tx=maj_size; else tz=maj_size; break;
          case 2: if (pass==0) tx=maj_size; else ty=maj_size; break;
        }
        tx *= invertx;
        ty *= inverty;
        tz *= invertz;
        double xs = xc - tx*maj_toff;
        double xe = xc + tx*(1. - maj_toff);
        double ys = yc - ty*maj_toff;
        double ye = yc + ty*(1. - maj_toff);
        double zs = zc - tz*maj_toff;
        double ze = zc + tz*(1. - maj_toff);

        worldannotator.AddLine(xs,ys,zs,
                                xe,ye,ze,
                                linewidth, color, infront);
      }

      double tx=0, ty=0, tz=0;
      const double s = 0.4;
      switch (axis)
      {
        case 0: ty=s*fontscale; tz=s*fontscale; break;
        case 1: tx=s*fontscale; tz=s*fontscale; break;
        case 2: tx=s*fontscale; ty=s*fontscale; break;
      }
      tx *= invertx;
      ty *= inverty;
      tz *= invertz;

#if 0
      char val[256];
      snprintf(val, 256, "%g", positions[i]);
      labels[i]->SetText(val);
      //if (fabs(positions[i]) < 1e-10)
      //    labels[i]->SetText("0");
      labels[i]->SetPosition(xc - tx, yc - ty, zc - tz);
      labels[i]->SetAlignment(TextAnnotation::HCenter,
                              TextAnnotation::VCenter);
#endif
    }

    // minor ticks
    CalculateTicks(lower, upper, true, positions, proportions, moreOrLessTickAdjustment);
    unsigned int nminor = (unsigned int)proportions.size();
    for (unsigned int i=0; i<nminor; ++i)
    {
      double xc = x0 + (x1-x0) * proportions[i];
      double yc = y0 + (y1-y0) * proportions[i];
      double zc = z0 + (z1-z0) * proportions[i];
      for (int pass=0; pass<=1; pass++)
      {
        double tx=0, ty=0, tz=0;
        switch (axis)
        {
          case 0: if (pass==0) ty=min_size; else tz=min_size; break;
          case 1: if (pass==0) tx=min_size; else tz=min_size; break;
          case 2: if (pass==0) tx=min_size; else ty=min_size; break;
        }
        tx *= invertx;
        ty *= inverty;
        tz *= invertz;
        double xs = xc - tx*min_toff;
        double xe = xc + tx*(1. - min_toff);
        double ys = yc - ty*min_toff;
        double ye = yc + ty*(1. - min_toff);
        double zs = zc - tz*min_toff;
        double ze = zc + tz*(1. - min_toff);

        worldannotator.AddLine(xs,ys,zs,
                               xe,ye,ze,
                               linewidth, color, infront);
      }
    }

#if 0
    for (unsigned int i=0; i<nmajor; ++i)
    {
      labels[i]->Render(view);
    }
#endif
  }
};


}} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation3D_h
