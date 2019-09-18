//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/AxisAnnotation2D.h>

#include <vtkm/rendering/TextAnnotationScreen.h>

#include <sstream>

namespace vtkm
{
namespace rendering
{

AxisAnnotation2D::AxisAnnotation2D()
  : AxisAnnotation()
{
  this->AlignH = TextAnnotation::HCenter;
  this->AlignV = TextAnnotation::VCenter;
  this->FontScale = 0.05f;
  this->LineWidth = 1.0;
  this->Color = vtkm::rendering::Color(1, 1, 1);
  this->Logarithmic = false;
  this->MoreOrLessTickAdjustment = 0;
}

AxisAnnotation2D::~AxisAnnotation2D()
{
}

void AxisAnnotation2D::SetRangeForAutoTicks(const Range& range)
{
  this->TickRange = range;

  if (this->Logarithmic)
  {
    CalculateTicksLogarithmic(this->TickRange, false, this->PositionsMajor, this->ProportionsMajor);
    CalculateTicksLogarithmic(this->TickRange, true, this->PositionsMinor, this->ProportionsMinor);
  }
  else
  {
    CalculateTicks(this->TickRange,
                   false,
                   this->PositionsMajor,
                   this->ProportionsMajor,
                   this->MoreOrLessTickAdjustment);
    CalculateTicks(this->TickRange,
                   true,
                   this->PositionsMinor,
                   this->ProportionsMinor,
                   this->MoreOrLessTickAdjustment);
  }
}

void AxisAnnotation2D::SetMajorTicks(const std::vector<vtkm::Float64>& pos,
                                     const std::vector<vtkm::Float64>& prop)
{
  this->PositionsMajor.clear();
  this->PositionsMajor.insert(this->PositionsMajor.begin(), pos.begin(), pos.end());

  this->ProportionsMajor.clear();
  this->ProportionsMajor.insert(this->ProportionsMajor.begin(), prop.begin(), prop.end());
}

void AxisAnnotation2D::SetMinorTicks(const std::vector<vtkm::Float64>& pos,
                                     const std::vector<vtkm::Float64>& prop)
{
  this->PositionsMinor.clear();
  this->PositionsMinor.insert(this->PositionsMinor.begin(), pos.begin(), pos.end());

  this->ProportionsMinor.clear();
  this->ProportionsMinor.insert(this->ProportionsMinor.begin(), prop.begin(), prop.end());
}

void AxisAnnotation2D::Render(const vtkm::rendering::Camera& camera,
                              const vtkm::rendering::WorldAnnotator& worldAnnotator,
                              vtkm::rendering::Canvas& canvas)
{
  canvas.AddLine(this->PosX0, this->PosY0, this->PosX1, this->PosY1, this->LineWidth, this->Color);

  // major ticks
  unsigned int nmajor = (unsigned int)this->ProportionsMajor.size();
  while (this->Labels.size() < nmajor)
  {
    this->Labels.push_back(
      std::unique_ptr<TextAnnotation>(new vtkm::rendering::TextAnnotationScreen(
        "test", this->Color, this->FontScale, vtkm::Vec2f_32(0, 0), 0)));
  }

  std::stringstream numberToString;
  for (unsigned int i = 0; i < nmajor; ++i)
  {
    vtkm::Float64 xc = this->PosX0 + (this->PosX1 - this->PosX0) * this->ProportionsMajor[i];
    vtkm::Float64 yc = this->PosY0 + (this->PosY1 - this->PosY0) * this->ProportionsMajor[i];
    vtkm::Float64 xs = xc - this->MajorTickSizeX * this->MajorTickOffset;
    vtkm::Float64 xe = xc + this->MajorTickSizeX * (1. - this->MajorTickOffset);
    vtkm::Float64 ys = yc - this->MajorTickSizeY * this->MajorTickOffset;
    vtkm::Float64 ye = yc + this->MajorTickSizeY * (1. - this->MajorTickOffset);

    canvas.AddLine(xs, ys, xe, ye, 1.0, this->Color);

    if (this->MajorTickSizeY == 0)
    {
      // slight shift to space between label and tick
      xs -= (this->MajorTickSizeX < 0 ? -1. : +1.) * this->FontScale * .1;
    }

    numberToString.str("");
    numberToString << this->PositionsMajor[i];

    this->Labels[i]->SetText(numberToString.str());
    //if (fabs(this->PositionsMajor[i]) < 1e-10)
    //    this->Labels[i]->SetText("0");
    TextAnnotation* tempBase = this->Labels[i].get();
    TextAnnotationScreen* tempDerived = static_cast<TextAnnotationScreen*>(tempBase);
    tempDerived->SetPosition(vtkm::Float32(xs), vtkm::Float32(ys));

    this->Labels[i]->SetAlignment(this->AlignH, this->AlignV);
  }

  // minor ticks
  if (this->MinorTickSizeX != 0 || this->MinorTickSizeY != 0)
  {
    unsigned int nminor = (unsigned int)this->ProportionsMinor.size();
    for (unsigned int i = 0; i < nminor; ++i)
    {
      vtkm::Float64 xc = this->PosX0 + (this->PosX1 - this->PosX0) * this->ProportionsMinor[i];
      vtkm::Float64 yc = this->PosY0 + (this->PosY1 - this->PosY0) * this->ProportionsMinor[i];
      vtkm::Float64 xs = xc - this->MinorTickSizeX * this->MinorTickOffset;
      vtkm::Float64 xe = xc + this->MinorTickSizeX * (1. - this->MinorTickOffset);
      vtkm::Float64 ys = yc - this->MinorTickSizeY * this->MinorTickOffset;
      vtkm::Float64 ye = yc + this->MinorTickSizeY * (1. - this->MinorTickOffset);

      canvas.AddLine(xs, ys, xe, ye, 1.0, this->Color);
    }
  }

  for (unsigned int i = 0; i < nmajor; ++i)
  {
    this->Labels[i]->Render(camera, worldAnnotator, canvas);
  }
}
}
} // namespace vtkm::rendering
