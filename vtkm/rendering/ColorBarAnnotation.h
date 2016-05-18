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
#ifndef vtk_m_rendering_ColorBarAnnotation_h
#define vtk_m_rendering_ColorBarAnnotation_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/RenderSurface.h>


namespace vtkm {
namespace rendering {

class ColorBarAnnotation
{
protected:
  ColorTable colortable;
  AxisAnnotation2D axis;
public:
  ColorBarAnnotation()
  {
  }
  void SetColorTable(ColorTable &ct)
  {
    colortable = ct;
  }
  void SetRange(double l, double h, int nticks)
  {
    std::vector<double> pos, prop;
    axis.SetMinorTicks(pos, prop); // clear any minor ticks

    for (int i=0; i<nticks; ++i)
    {
      double p = double(i) / double(nticks-1);
      double v = l + p*(h-l);
      pos.push_back(v);
      prop.push_back(p);
    }
    axis.SetMajorTicks(pos, prop);
  }

  virtual void Render(View &view,
                      WorldAnnotator &worldAnnotator,
                      RenderSurface &renderSurface)
  {
    float l = -0.88f, r = +0.88f;
    float b = +0.87f, t = +0.92f;

    renderSurface.AddColorBar(l, t, r-l, b-t,
                              colortable, true);

    axis.SetColor(Color(1,1,1));
    axis.SetLineWidth(1);
    axis.SetScreenPosition(l,b, r,b);
    axis.SetMajorTickSize(0, .02, 1.0);
    axis.SetMinorTickSize(0,0,0); // no minor ticks
    axis.Render(view, worldAnnotator, renderSurface);
  }
};

}} //namespace vtkm::rendering

#endif // vtk_m_rendering_ColorBarAnnotation_h
