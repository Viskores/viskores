//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_AxisAnnotation3D_h
#define vtk_m_rendering_AxisAnnotation3D_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Range.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/AxisAnnotation.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/TextAnnotationBillboard.h>
#include <vtkm/rendering/WorldAnnotator.h>

#include <sstream>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT AxisAnnotation3D : public AxisAnnotation
{
private:
protected:
  vtkm::Float64 TickMajorSize, TickMajorOffset;
  vtkm::Float64 TickMinorSize, TickMinorOffset;
  int Axis;
  vtkm::Vec3f_32 Invert;
  vtkm::Vec3f_64 Point0, Point1;
  vtkm::Range Range;
  vtkm::Float64 FontScale;
  vtkm::Float32 FontOffset;
  vtkm::Float32 LineWidth;
  vtkm::rendering::Color Color;
  std::vector<std::unique_ptr<TextAnnotationBillboard>> Labels;
  int MoreOrLessTickAdjustment;

public:
  AxisAnnotation3D();

  ~AxisAnnotation3D();

  AxisAnnotation3D(const AxisAnnotation3D&) = delete;

  AxisAnnotation3D& operator=(const AxisAnnotation3D&) = delete;

  VTKM_CONT
  void SetMoreOrLessTickAdjustment(int offset) { this->MoreOrLessTickAdjustment = offset; }

  VTKM_CONT
  void SetColor(vtkm::rendering::Color c) { this->Color = c; }

  VTKM_CONT
  void SetAxis(int a) { this->Axis = a; }

  void SetTickInvert(bool x, bool y, bool z);

  /// offset of 0 means the tick is inside the frame
  /// offset of 1 means the tick is outside the frame
  /// offset of 0.5 means the tick is centered on the frame
  VTKM_CONT
  void SetMajorTickSize(vtkm::Float64 size, vtkm::Float64 offset)
  {
    this->TickMajorSize = size;
    this->TickMajorOffset = offset;
  }
  VTKM_CONT
  void SetMinorTickSize(vtkm::Float64 size, vtkm::Float64 offset)
  {
    this->TickMinorSize = size;
    this->TickMinorOffset = offset;
  }

  VTKM_CONT
  void SetWorldPosition(const vtkm::Vec3f_64& point0, const vtkm::Vec3f_64& point1)
  {
    this->Point0 = point0;
    this->Point1 = point1;
  }

  VTKM_CONT
  void SetWorldPosition(vtkm::Float64 x0,
                        vtkm::Float64 y0,
                        vtkm::Float64 z0,
                        vtkm::Float64 x1,
                        vtkm::Float64 y1,
                        vtkm::Float64 z1)
  {
    this->SetWorldPosition(vtkm::make_Vec(x0, y0, z0), vtkm::make_Vec(x1, y1, z1));
  }

  void SetLabelFontScale(vtkm::Float64 s);

  void SetLabelFontOffset(vtkm::Float32 off) { this->FontOffset = off; }

  void SetRange(const vtkm::Range& range) { this->Range = range; }

  void SetRange(vtkm::Float64 lower, vtkm::Float64 upper)
  {
    this->SetRange(vtkm::Range(lower, upper));
  }

  virtual void Render(const vtkm::rendering::Camera& camera,
                      const vtkm::rendering::WorldAnnotator& worldAnnotator,
                      vtkm::rendering::Canvas& canvas) override;
};
}
} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation3D_h
