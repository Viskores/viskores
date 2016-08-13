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

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>
#include <vtkm/rendering/AxisAnnotation.h>
#include <vtkm/rendering/TextAnnotation.h>

namespace vtkm {
namespace rendering {

class AxisAnnotation3D : public AxisAnnotation
{
private:
protected:
  vtkm::Float64 maj_size, maj_toff;
  vtkm::Float64 min_size, min_toff;
  int    axis;
  vtkm::Float32 invertx, inverty, invertz;
  vtkm::Float64 x0, y0, z0,   x1, y1, z1;
  vtkm::Float64 lower, upper;
  vtkm::Float64 fontscale;
  vtkm::Float32 fontoffset;
  vtkm::Float32  linewidth;
  vtkm::rendering::Color  color;
  std::vector<BillboardTextAnnotation*> labels;
  int moreOrLessTickAdjustment;
public:
  AxisAnnotation3D() : AxisAnnotation()
  {
    axis = 0;
    color = Color(1,1,1);
    fontoffset = 0.1f; // world space offset from axis
    fontscale = 0.05; // screen space font size
    linewidth = 1.0;
    color = Color(1,1,1);
    moreOrLessTickAdjustment = 0;
  }
  virtual ~AxisAnnotation3D()
  {
  }
  void SetMoreOrLessTickAdjustment(int offset)
  {
    moreOrLessTickAdjustment = offset;
  }
  void SetColor(vtkm::rendering::Color c)
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
  void SetMajorTickSize(vtkm::Float64 size, vtkm::Float64 offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    maj_size = size;
    maj_toff = offset;
  }
  void SetMinorTickSize(vtkm::Float64 size, vtkm::Float64 offset)
  {
    min_size = size;
    min_toff = offset;
  }
  void SetWorldPosition(vtkm::Float64 x0_, vtkm::Float64 y0_, vtkm::Float64 z0_,
                        vtkm::Float64 x1_, vtkm::Float64 y1_, vtkm::Float64 z1_)
  {
    x0 = x0_;
    y0 = y0_;
    z0 = z0_;

    x1 = x1_;
    y1 = y1_;
    z1 = z1_;
  }
  void SetLabelFontScale(vtkm::Float64 s)
  {
    fontscale = s;
    for (unsigned int i=0; i<labels.size(); i++)
      labels[i]->SetScale(vtkm::Float32(s));
  }
  void SetLabelFontOffset(vtkm::Float32 off)
  {
    fontoffset = off;
  }
  void SetRange(vtkm::Float64 l, vtkm::Float64 u)
  {
    lower = l;
    upper = u;
  }
  virtual void Render(const vtkm::rendering::Camera &camera,
                      const vtkm::rendering::WorldAnnotator &worldAnnotator,
                      vtkm::rendering::Canvas &canvas)
  {
    bool infront = true;
    worldAnnotator.AddLine(x0,y0,z0,
                            x1,y1,z1,
                            linewidth, color, infront);

    std::vector<vtkm::Float64> positions;
    std::vector<vtkm::Float64> proportions;
    // major ticks
    CalculateTicks(lower, upper, false, positions, proportions, moreOrLessTickAdjustment);
    unsigned int nmajor = (unsigned int)proportions.size();
    while (labels.size() < nmajor)
    {
      labels.push_back(new BillboardTextAnnotation("test",
                                                   color,
                                                   vtkm::Float32(fontscale),
                                                   0,0,0,
                                                   0));
    }

    for (unsigned int i=0; i<nmajor; ++i)
    {
      vtkm::Float64 xc = x0 + (x1-x0) * proportions[i];
      vtkm::Float64 yc = y0 + (y1-y0) * proportions[i];
      vtkm::Float64 zc = z0 + (z1-z0) * proportions[i];
      for (int pass=0; pass<=1; pass++)
      {
        vtkm::Float64 tx=0, ty=0, tz=0;
        switch (axis)
        {
          case 0: if (pass==0) ty=maj_size; else tz=maj_size; break;
          case 1: if (pass==0) tx=maj_size; else tz=maj_size; break;
          case 2: if (pass==0) tx=maj_size; else ty=maj_size; break;
        }
        tx *= invertx;
        ty *= inverty;
        tz *= invertz;
        vtkm::Float64 xs = xc - tx*maj_toff;
        vtkm::Float64 xe = xc + tx*(1. - maj_toff);
        vtkm::Float64 ys = yc - ty*maj_toff;
        vtkm::Float64 ye = yc + ty*(1. - maj_toff);
        vtkm::Float64 zs = zc - tz*maj_toff;
        vtkm::Float64 ze = zc + tz*(1. - maj_toff);

        worldAnnotator.AddLine(xs,ys,zs,
                               xe,ye,ze,
                               linewidth, color, infront);
      }

      vtkm::Float32 tx=0, ty=0, tz=0;
      const vtkm::Float32 s = 0.4f;
      switch (axis)
      {
        case 0: ty=s*fontoffset; tz=s*fontoffset; break;
        case 1: tx=s*fontoffset; tz=s*fontoffset; break;
        case 2: tx=s*fontoffset; ty=s*fontoffset; break;
      }
      tx *= invertx;
      ty *= inverty;
      tz *= invertz;

      char val[256];
      snprintf(val, 256, "%g", positions[i]);
      labels[i]->SetText(val);
      //if (fabs(positions[i]) < 1e-10)
      //    labels[i]->SetText("0");
      labels[i]->SetPosition(vtkm::Float32(xc - tx),
                             vtkm::Float32(yc - ty),
                             vtkm::Float32(zc - tz));
      labels[i]->SetAlignment(TextAnnotation::HCenter,
                              TextAnnotation::VCenter);
    }

    // minor ticks
    CalculateTicks(lower, upper, true, positions, proportions, moreOrLessTickAdjustment);
    unsigned int nminor = (unsigned int)proportions.size();
    for (unsigned int i=0; i<nminor; ++i)
    {
      vtkm::Float64 xc = x0 + (x1-x0) * proportions[i];
      vtkm::Float64 yc = y0 + (y1-y0) * proportions[i];
      vtkm::Float64 zc = z0 + (z1-z0) * proportions[i];
      for (int pass=0; pass<=1; pass++)
      {
        vtkm::Float64 tx=0, ty=0, tz=0;
        switch (axis)
        {
          case 0: if (pass==0) ty=min_size; else tz=min_size; break;
          case 1: if (pass==0) tx=min_size; else tz=min_size; break;
          case 2: if (pass==0) tx=min_size; else ty=min_size; break;
        }
        tx *= invertx;
        ty *= inverty;
        tz *= invertz;
        vtkm::Float64 xs = xc - tx*min_toff;
        vtkm::Float64 xe = xc + tx*(1. - min_toff);
        vtkm::Float64 ys = yc - ty*min_toff;
        vtkm::Float64 ye = yc + ty*(1. - min_toff);
        vtkm::Float64 zs = zc - tz*min_toff;
        vtkm::Float64 ze = zc + tz*(1. - min_toff);

        worldAnnotator.AddLine(xs,ys,zs,
                               xe,ye,ze,
                               linewidth, color, infront);
      }
    }

    for (unsigned int i=0; i<nmajor; ++i)
    {
      labels[i]->Render(camera, worldAnnotator, canvas);
    }
  }
};


}} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation3D_h
