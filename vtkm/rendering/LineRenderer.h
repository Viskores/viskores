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

#ifndef vtk_m_rendering_LineRenderer_h
#define vtk_m_rendering_LineRenderer_h

#include <vtkm/Matrix.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT LineRenderer
{
public:
  VTKM_CONT
  LineRenderer(const vtkm::rendering::Canvas* canvas, vtkm::Matrix<vtkm::Float32, 4, 4> transform);

  VTKM_CONT
  void RenderLine(const vtkm::Vec<vtkm::Float64, 2>& point0,
                  const vtkm::Vec<vtkm::Float64, 2>& point1,
                  vtkm::Float32 lineWidth,
                  const vtkm::rendering::Color& color);

  VTKM_CONT
  void RenderLine(const vtkm::Vec<vtkm::Float64, 3>& point0,
                  const vtkm::Vec<vtkm::Float64, 3>& point1,
                  vtkm::Float32 lineWidth,
                  const vtkm::rendering::Color& color);

private:
  VTKM_CONT
  vtkm::Vec<vtkm::Float32, 3> TransformPoint(const vtkm::Vec<vtkm::Float64, 3>& point) const;

  const vtkm::rendering::Canvas* Canvas;
  vtkm::Matrix<vtkm::Float32, 4, 4> Transform;
}; // class LineRenderer
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_LineRenderer_h
