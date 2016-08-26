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
#ifndef vtk_m_rendering_WorldAnnotator_h
#define vtk_m_rendering_WorldAnnotator_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Color.h>

namespace vtkm {
namespace rendering {

class WorldAnnotator
{
public:
  virtual ~WorldAnnotator() {  }

  virtual void AddLine(const vtkm::Vec<vtkm::Float64,3> &vtkmNotUsed(point0),
                       const vtkm::Vec<vtkm::Float64,3> &vtkmNotUsed(point1),
                       vtkm::Float32 vtkmNotUsed(lineWidth),
                       const vtkm::rendering::Color &vtkmNotUsed(color),
                       bool vtkmNotUsed(inFront) = false) const {}
  void AddLine(vtkm::Float64 x0, vtkm::Float64 y0, vtkm::Float64 z0,
               vtkm::Float64 x1, vtkm::Float64 y1, vtkm::Float64 z1,
               vtkm::Float32 lineWidth,
               const vtkm::rendering::Color &color,
               bool inFront = false) const
  {
    this->AddLine(vtkm::make_Vec(x0,y0,z0),
                  vtkm::make_Vec(x1,y1,z1),
                  lineWidth,
                  color,
                  inFront);
  }

  virtual void AddText(const vtkm::Vec<vtkm::Float32,3> &vtkmNotUsed(origin),
                       const vtkm::Vec<vtkm::Float32,3> &vtkmNotUsed(right),
                       const vtkm::Vec<vtkm::Float32,3> &vtkmNotUsed(up),
                       vtkm::Float32 vtkmNotUsed(scale),
                       const vtkm::Vec<vtkm::Float32,2> &vtkmNotUsed(anchor),
                       const vtkm::rendering::Color &vtkmNotUsed(color),
                       const std::string &vtkmNotUsed(text)) const {  }
  void AddText(vtkm::Float32 originX,
               vtkm::Float32 originY,
               vtkm::Float32 originZ,
               vtkm::Float32 rightX,
               vtkm::Float32 rightY,
               vtkm::Float32 rightZ,
               vtkm::Float32 upX,
               vtkm::Float32 upY,
               vtkm::Float32 upZ,
               vtkm::Float32 scale,
               vtkm::Float32 anchorX,
               vtkm::Float32 anchorY,
               const vtkm::rendering::Color &color,
               const std::string &text) const
  {
    this->AddText(vtkm::make_Vec(originX, originY, originZ),
                  vtkm::make_Vec(rightX, rightY, rightZ),
                  vtkm::make_Vec(upX, upY, upZ),
                  scale,
                  vtkm::make_Vec(anchorX, anchorY),
                  color,
                  text);
  }
};

}} //namespace vtkm::rendering

#endif // vtk_m_rendering_WorldAnnotator_h
