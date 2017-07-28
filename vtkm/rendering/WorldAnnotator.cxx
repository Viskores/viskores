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

#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm
{
namespace rendering
{

WorldAnnotator::~WorldAnnotator()
{
}

void WorldAnnotator::AddLine(const vtkm::Vec<vtkm::Float64, 3>& vtkmNotUsed(point0),
                             const vtkm::Vec<vtkm::Float64, 3>& vtkmNotUsed(point1),
                             vtkm::Float32 vtkmNotUsed(lineWidth),
                             const vtkm::rendering::Color& vtkmNotUsed(color),
                             bool vtkmNotUsed(inFront)) const
{
  // Default implementation does nothing. Should this be pure virtual and force
  // all subclasses to implement this? We would have to implement a
  // WorldAnnotator for ray tracing first.
}

void WorldAnnotator::AddText(const vtkm::Vec<vtkm::Float32, 3>& vtkmNotUsed(origin),
                             const vtkm::Vec<vtkm::Float32, 3>& vtkmNotUsed(right),
                             const vtkm::Vec<vtkm::Float32, 3>& vtkmNotUsed(up),
                             vtkm::Float32 vtkmNotUsed(scale),
                             const vtkm::Vec<vtkm::Float32, 2>& vtkmNotUsed(anchor),
                             const vtkm::rendering::Color& vtkmNotUsed(color),
                             const std::string& vtkmNotUsed(text)) const
{
  // Default implementation does nothing. Should this be pure virtual and force
  // all subclasses to implement this? We would have to implement a
  // WorldAnnotator for ray tracing first.
}
}
} // namespace vtkm::rendering
