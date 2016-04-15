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
#ifndef vtk_m_rendering_BoundingBoxAnnotation_h
#define vtk_m_rendering_BoundingBoxAnnotation_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm {
namespace rendering {

class BoundingBoxAnnotation
{
private:
  Color color;
  double dmin[3];
  double dmax[3];
public:
  BoundingBoxAnnotation()
  {
    dmin[0] = dmin[1] = dmin[2] = -1;
    dmax[0] = dmax[1] = dmax[2] = +1;
    color = Color(.5,.5,.5);
  }
  virtual ~BoundingBoxAnnotation()
  {
  }
  void SetExtents(double bounds[6])
  {
    SetExtents(bounds[0], bounds[1],
               bounds[2], bounds[3],
               bounds[4], bounds[5]);
  }
  void SetExtents(double xmin, double xmax,
                  double ymin, double ymax,
                  double zmin, double zmax)
  {
    dmin[0] = xmin;
    dmax[0] = xmax;
    dmin[1] = ymin;
    dmax[1] = ymax;
    dmin[2] = zmin;
    dmax[2] = zmax;
  }
  void SetColor(Color c)
  {
    color = c;
  }
  virtual void Render(View3D &view,
                      WorldAnnotator &annotator)
  {
    //win->SetupForWorldSpace();

    float linewidth = 1.0;

    annotator.AddLine(dmin[0],dmin[1],dmin[2],
                      dmin[0],dmin[1],dmax[2],
                      linewidth, color);
    annotator.AddLine(dmin[0],dmax[1],dmin[2],
                      dmin[0],dmax[1],dmax[2],
                      linewidth, color);
    annotator.AddLine(dmax[0],dmin[1],dmin[2],
                      dmax[0],dmin[1],dmax[2],
                      linewidth, color);
    annotator.AddLine(dmax[0],dmax[1],dmin[2],
                      dmax[0],dmax[1],dmax[2],
                      linewidth, color);

    annotator.AddLine(dmin[0],dmin[1],dmin[2],
                      dmin[0],dmax[1],dmin[2],
                      linewidth, color);
    annotator.AddLine(dmin[0],dmin[1],dmax[2],
                      dmin[0],dmax[1],dmax[2],
                      linewidth, color);
    annotator.AddLine(dmax[0],dmin[1],dmin[2],
                      dmax[0],dmax[1],dmin[2],
                      linewidth, color);
    annotator.AddLine(dmax[0],dmin[1],dmax[2],
                      dmax[0],dmax[1],dmax[2],
                      linewidth, color);

    annotator.AddLine(dmin[0],dmin[1],dmin[2],
                      dmax[0],dmin[1],dmin[2],
                      linewidth, color);
    annotator.AddLine(dmin[0],dmin[1],dmax[2],
                      dmax[0],dmin[1],dmax[2],
                      linewidth, color);
    annotator.AddLine(dmin[0],dmax[1],dmin[2],
                      dmax[0],dmax[1],dmin[2],
                      linewidth, color);
    annotator.AddLine(dmin[0],dmax[1],dmax[2],
                      dmax[0],dmax[1],dmax[2],
                      linewidth, color);
  }
};


}} //namespace vtkm::rendering

#endif // vtk_m_rendering_BoundingBoxAnnotation_h

