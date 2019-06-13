//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_ColorBarAnnotation_h
#define vtk_m_rendering_ColorBarAnnotation_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/AxisAnnotation2D.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT ColorBarAnnotation
{
protected:
  vtkm::cont::ColorTable ColorTable;
  vtkm::rendering::AxisAnnotation2D Axis;
  vtkm::Bounds Position;
  bool Horizontal;
  std::string FieldName;

public:
  ColorBarAnnotation();

  virtual ~ColorBarAnnotation();

  VTKM_CONT
  void SetColorTable(const vtkm::cont::ColorTable& colorTable) { this->ColorTable = colorTable; }

  VTKM_CONT
  void SetRange(const vtkm::Range& range, vtkm::IdComponent numTicks);

  VTKM_CONT
  void SetFieldName(const std::string& fieldName);

  VTKM_CONT
  void SetRange(vtkm::Float64 l, vtkm::Float64 h, vtkm::IdComponent numTicks)
  {
    this->SetRange(vtkm::Range(l, h), numTicks);
  }


  VTKM_CONT
  void SetPosition(const vtkm::Bounds& position);

  virtual void Render(const vtkm::rendering::Camera& camera,
                      const vtkm::rendering::WorldAnnotator& worldAnnotator,
                      vtkm::rendering::Canvas& canvas);
};
}
} //namespace vtkm::rendering

#endif // vtk_m_rendering_ColorBarAnnotation_h
