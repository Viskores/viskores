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
#ifndef vtk_m_rendering_CanvasRayTracer_h
#define vtk_m_rendering_CanvasRayTracer_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Canvas.h>
namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT CanvasRayTracer : public Canvas
{
public:
  CanvasRayTracer(vtkm::Id width = 1024, vtkm::Id height = 1024);

  ~CanvasRayTracer();

  void Initialize() VTKM_OVERRIDE;

  void Activate() VTKM_OVERRIDE;

  void Finish() VTKM_OVERRIDE;

  void Clear() VTKM_OVERRIDE;

  vtkm::rendering::Canvas* NewCopy() const VTKM_OVERRIDE;

  void WriteToCanvas(const vtkm::cont::ArrayHandle<vtkm::Id>& pixelIds,
                     const vtkm::cont::ArrayHandle<vtkm::Float32>& distances,
                     const vtkm::cont::ArrayHandle<vtkm::Float32>& colors,
                     const vtkm::rendering::Camera& camera);

  void WriteToCanvas(const vtkm::cont::ArrayHandle<vtkm::Id>& pixelIds,
                     const vtkm::cont::ArrayHandle<vtkm::Float64>& distances,
                     const vtkm::cont::ArrayHandle<vtkm::Float64>& colors,
                     const vtkm::rendering::Camera& camera);

  void AddLine(const vtkm::Vec<vtkm::Float64, 2>& point0,
               const vtkm::Vec<vtkm::Float64, 2>& point1,
               vtkm::Float32 linewidth,
               const vtkm::rendering::Color& color) const VTKM_OVERRIDE;

  void AddColorBar(const vtkm::Bounds& bounds,
                   const vtkm::rendering::ColorTable& colorTable,
                   bool horizontal) const VTKM_OVERRIDE;


  void AddText(const vtkm::Vec<vtkm::Float32, 2>& position,
               vtkm::Float32 scale,
               vtkm::Float32 angle,
               vtkm::Float32 windowAspect,
               const vtkm::Vec<vtkm::Float32, 2>& anchor,
               const vtkm::rendering::Color& color,
               const std::string& text) const VTKM_OVERRIDE;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_CanvasRayTracer_h
