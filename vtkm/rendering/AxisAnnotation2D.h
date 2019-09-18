//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_AxisAnnotation2D_h
#define vtk_m_rendering_AxisAnnotation2D_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Range.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/AxisAnnotation.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/TextAnnotation.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT AxisAnnotation2D : public AxisAnnotation
{
protected:
  vtkm::Float64 MajorTickSizeX, MajorTickSizeY, MajorTickOffset;
  vtkm::Float64 MinorTickSizeX, MinorTickSizeY, MinorTickOffset;
  vtkm::Float64 PosX0, PosY0, PosX1, PosY1;
  vtkm::Range TickRange;
  vtkm::Float32 FontScale;
  vtkm::Float32 LineWidth;
  vtkm::rendering::Color Color;
  bool Logarithmic;

  TextAnnotation::HorizontalAlignment AlignH;
  TextAnnotation::VerticalAlignment AlignV;
  std::vector<std::unique_ptr<TextAnnotation>> Labels;
  //  std::vector<TextAnnotation*> Labels;

  std::vector<vtkm::Float64> PositionsMajor;
  std::vector<vtkm::Float64> ProportionsMajor;

  std::vector<vtkm::Float64> PositionsMinor;
  std::vector<vtkm::Float64> ProportionsMinor;

  int MoreOrLessTickAdjustment;

public:
  AxisAnnotation2D();

  ~AxisAnnotation2D();

  AxisAnnotation2D(const AxisAnnotation2D&) = delete;

  AxisAnnotation2D& operator=(const AxisAnnotation2D&) = delete;

  void SetLogarithmic(bool l) { this->Logarithmic = l; }

  void SetMoreOrLessTickAdjustment(int offset) { this->MoreOrLessTickAdjustment = offset; }

  void SetColor(vtkm::rendering::Color c) { this->Color = c; }

  void SetLineWidth(vtkm::Float32 lw) { this->LineWidth = lw; }

  void SetMajorTickSize(vtkm::Float64 xlen, vtkm::Float64 ylen, vtkm::Float64 offset)
  {
    /// offset of 0 means the tick is inside the frame
    /// offset of 1 means the tick is outside the frame
    /// offset of 0.5 means the tick is centered on the frame
    this->MajorTickSizeX = xlen;
    this->MajorTickSizeY = ylen;
    this->MajorTickOffset = offset;
  }

  void SetMinorTickSize(vtkm::Float64 xlen, vtkm::Float64 ylen, vtkm::Float64 offset)
  {
    this->MinorTickSizeX = xlen;
    this->MinorTickSizeY = ylen;
    this->MinorTickOffset = offset;
  }

  ///\todo: rename, since it might be screen OR world position?
  void SetScreenPosition(vtkm::Float64 x0, vtkm::Float64 y0, vtkm::Float64 x1, vtkm::Float64 y1)
  {
    this->PosX0 = x0;
    this->PosY0 = y0;

    this->PosX1 = x1;
    this->PosY1 = y1;
  }

  void SetLabelAlignment(TextAnnotation::HorizontalAlignment h, TextAnnotation::VerticalAlignment v)
  {
    this->AlignH = h;
    this->AlignV = v;
  }

  void SetLabelFontScale(vtkm::Float32 s)
  {
    this->FontScale = s;
    for (unsigned int i = 0; i < this->Labels.size(); i++)
      this->Labels[i]->SetScale(s);
  }

  void SetRangeForAutoTicks(const vtkm::Range& range);
  void SetRangeForAutoTicks(vtkm::Float64 lower, vtkm::Float64 upper)
  {
    this->SetRangeForAutoTicks(vtkm::Range(lower, upper));
  }

  void SetMajorTicks(const std::vector<vtkm::Float64>& positions,
                     const std::vector<vtkm::Float64>& proportions);

  void SetMinorTicks(const std::vector<vtkm::Float64>& positions,
                     const std::vector<vtkm::Float64>& proportions);

  void Render(const vtkm::rendering::Camera& camera,
              const vtkm::rendering::WorldAnnotator& worldAnnotator,
              vtkm::rendering::Canvas& canvas) override;
};
}
} //namespace vtkm::rendering

#endif // vtk_m_rendering_AxisAnnotation2D_h
