//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_CanvasWireframer_h
#define vtk_m_rendering_CanvasWireframer_h

#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT CanvasWireframer : public Canvas
{
public:
  CanvasWireframer(vtkm::Id width = 1024, vtkm::Id height = 1024);

  ~CanvasWireframer();

  void Initialize() VTKM_OVERRIDE;

  void Activate() VTKM_OVERRIDE;

  void Finish() VTKM_OVERRIDE;

  void Clear() VTKM_OVERRIDE;

  vtkm::rendering::Canvas* NewCopy() const VTKM_OVERRIDE;

  void AddLine(const vtkm::Vec<vtkm::Float64, 2>& start,
               const vtkm::Vec<vtkm::Float64, 2>& end,
               vtkm::Float32 linewidth,
               const vtkm::rendering::Color& color) const VTKM_OVERRIDE;

  virtual void AddColorBar(const vtkm::Bounds& bounds,
                           const vtkm::rendering::ColorTable& colorTable,
                           bool horizontal) const VTKM_OVERRIDE;

  virtual void AddText(const vtkm::Vec<vtkm::Float32, 2>& position,
                       vtkm::Float32 scale,
                       vtkm::Float32 angle,
                       vtkm::Float32 windowAspect,
                       const vtkm::Vec<vtkm::Float32, 2>& anchor,
                       const vtkm::rendering::Color& color,
                       const std::string& text) const VTKM_OVERRIDE;

  virtual void AddColorSwatch(const vtkm::Vec<vtkm::Float64, 2>& point0,
                              const vtkm::Vec<vtkm::Float64, 2>& point1,
                              const vtkm::Vec<vtkm::Float64, 2>& point2,
                              const vtkm::Vec<vtkm::Float64, 2>& point3,
                              const vtkm::rendering::Color& color) const VTKM_OVERRIDE;

private:
  vtkm::Id GetBufferIndex(vtkm::Id x, vtkm::Id y) const;
  void BlendPixel(vtkm::Float32 x,
                  vtkm::Float32 y,
                  const vtkm::rendering::Color& color,
                  vtkm::Float32 intensity) const;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasWireframer_h
