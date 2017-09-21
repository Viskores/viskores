//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/ColorBarAnnotation.h>

namespace vtkm
{
namespace rendering
{

ColorBarAnnotation::ColorBarAnnotation()
{
}

ColorBarAnnotation::~ColorBarAnnotation()
{
}

void ColorBarAnnotation::SetRange(const vtkm::Range& range, vtkm::IdComponent numTicks)
{
  std::vector<vtkm::Float64> positions, proportions;
  this->Axis.SetMinorTicks(positions, proportions); // clear any minor ticks

  for (vtkm::IdComponent i = 0; i < numTicks; ++i)
  {
    vtkm::Float64 prop = static_cast<vtkm::Float64>(i) / static_cast<vtkm::Float64>(numTicks - 1);
    vtkm::Float64 pos = range.Min + prop * range.Length();
    positions.push_back(pos);
    proportions.push_back(prop);
  }
  this->Axis.SetMajorTicks(positions, proportions);
}

void ColorBarAnnotation::Render(const vtkm::rendering::Camera& camera,
                                const vtkm::rendering::WorldAnnotator& worldAnnotator,
                                vtkm::rendering::Canvas& canvas)
{
  vtkm::Bounds bounds(vtkm::Range(-0.88, +0.88), vtkm::Range(+0.87, +0.92), vtkm::Range(0, 0));

  canvas.AddColorBar(bounds, this->ColorTable, true);

  this->Axis.SetColor(vtkm::rendering::Color(1, 1, 1));
  this->Axis.SetLineWidth(1);
  this->Axis.SetScreenPosition(bounds.X.Min, bounds.Y.Min, bounds.X.Max, bounds.Y.Min);
  this->Axis.SetMajorTickSize(0, .02, 1.0);
  this->Axis.SetMinorTickSize(0, 0, 0); // no minor ticks
  this->Axis.SetLabelAlignment(TextAnnotation::HCenter, TextAnnotation::Top);
  this->Axis.Render(camera, worldAnnotator, canvas);
}
}
} // namespace vtkm::rendering
