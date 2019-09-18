//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_AxisAnnotation_h
#define vtk_m_rendering_AxisAnnotation_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT AxisAnnotation
{
protected:
  void CalculateTicks(const vtkm::Range& range,
                      bool minor,
                      std::vector<vtkm::Float64>& positions,
                      std::vector<vtkm::Float64>& proportions,
                      int modifyTickQuantity) const;
  void CalculateTicksLogarithmic(const vtkm::Range& range,
                                 bool minor,
                                 std::vector<vtkm::Float64>& positions,
                                 std::vector<vtkm::Float64>& proportions) const;

public:
  AxisAnnotation();

  virtual ~AxisAnnotation();

  virtual void Render(const vtkm::rendering::Camera& camera,
                      const vtkm::rendering::WorldAnnotator& worldAnnotator,
                      vtkm::rendering::Canvas& canvas) = 0;
};
}
} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation_h
