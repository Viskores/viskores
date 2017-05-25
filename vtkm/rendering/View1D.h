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
#ifndef vtk_m_rendering_View1D_h
#define vtk_m_rendering_View1D_h

#include <vtkm/rendering/AxisAnnotation2D.h>
#include <vtkm/rendering/View.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT View1D : public vtkm::rendering::View
{
public:
  View1D(const vtkm::rendering::Scene& scene, const vtkm::rendering::Mapper& mapper,
         const vtkm::rendering::Canvas& canvas,
         const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1));

  View1D(const vtkm::rendering::Scene& scene, const vtkm::rendering::Mapper& mapper,
         const vtkm::rendering::Canvas& canvas, const vtkm::rendering::Camera& camera,
         const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1));

  ~View1D();

  void Paint() VTKM_OVERRIDE;
  void RenderScreenAnnotations() VTKM_OVERRIDE;
  void RenderWorldAnnotations() VTKM_OVERRIDE;

private:
  // 1D-specific annotations
  vtkm::rendering::AxisAnnotation2D HorizontalAxisAnnotation;
  vtkm::rendering::AxisAnnotation2D VerticalAxisAnnotation;
};
}
} // namespace vtkm::rendering

#endif //vtk_m_rendering_View1D_h
