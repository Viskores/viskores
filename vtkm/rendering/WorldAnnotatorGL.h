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
#ifndef vtk_m_rendering_WorldAnnotatorGL_h
#define vtk_m_rendering_WorldAnnotatorGL_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm {
namespace rendering {

class WorldAnnotatorGL : public WorldAnnotator
{
public:
  virtual void AddLine(vtkm::Float64 x0, vtkm::Float64 y0, vtkm::Float64 z0,
                       vtkm::Float64 x1, vtkm::Float64 y1, vtkm::Float64 z1,
                       vtkm::Float32 linewidth,
                       const vtkm::rendering::Color &c,
                       bool infront)
  {
    if (infront)
      glDepthRange(-.0001,.9999);

    glDisable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glColor3fv(c.Components);

    glLineWidth(linewidth);

    glBegin(GL_LINES);
    glVertex3d(x0,y0,z0);
    glVertex3d(x1,y1,z1);
    glEnd();

    if (infront)
      glDepthRange(0,1);

  }
};

}} //namespace vtkm::rendering

#endif // vtk_m_rendering_WorldAnnotatorGL_h
