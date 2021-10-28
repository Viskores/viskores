//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_rendering_LineRendererBatcher_h
#define vtk_m_rendering_LineRendererBatcher_h

#include <string>
#include <vector>

#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT LineRendererBatcher
{
public:
  VTKM_CONT
  LineRendererBatcher();

  VTKM_CONT
  void BatchLine(const vtkm::Vec3f_64& start,
                 const vtkm::Vec3f_64& end,
                 const vtkm::rendering::Color& color);

  VTKM_CONT
  void BatchLine(const vtkm::Vec3f_32& start,
                 const vtkm::Vec3f_32& end,
                 const vtkm::rendering::Color& color);

  void Render(const vtkm::rendering::Canvas* canvas) const;

private:
  std::vector<vtkm::Vec3f_32> Starts;
  std::vector<vtkm::Vec3f_32> Ends;
  std::vector<vtkm::Vec4f_32> Colors;
};
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_LineRendererBatcher_h
