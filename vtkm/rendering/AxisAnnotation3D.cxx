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

#include <vtkm/rendering/AxisAnnotation3D.h>

namespace vtkm
{
namespace rendering
{

AxisAnnotation3D::AxisAnnotation3D()
  : AxisAnnotation()
  , TickMajorSize(1.0)
  , TickMajorOffset(1.0)
  , TickMinorSize(0.1)
  , TickMinorOffset(1.0)
  , Axis(0)
  , Invert(1, 1, 1)
  , Point0(0, 0, 0)
  , Point1(1, 0, 0)
  , Range(0, 1)
  , FontScale(0.05f)
  , // screen space font size
  FontOffset(0.1f)
  , // world space offset from axis
  LineWidth(1.0)
  , Color(1, 1, 1)
  , MoreOrLessTickAdjustment(0)
{
}

AxisAnnotation3D::~AxisAnnotation3D()
{
}

void AxisAnnotation3D::SetTickInvert(bool x, bool y, bool z)
{
  this->Invert[0] = x ? +1.0f : -1.0f;
  this->Invert[1] = y ? +1.0f : -1.0f;
  this->Invert[2] = z ? +1.0f : -1.0f;
}

void AxisAnnotation3D::SetLabelFontScale(Float64 s)
{
  this->FontScale = s;
  for (unsigned int i = 0; i < this->Labels.size(); i++)
  {
    this->Labels[i]->SetScale(vtkm::Float32(s));
  }
}

void AxisAnnotation3D::Render(const Camera& camera,
                              const WorldAnnotator& worldAnnotator,
                              Canvas& canvas)
{
  bool infront = true;
  worldAnnotator.AddLine(this->Point0, this->Point1, this->LineWidth, this->Color, infront);

  std::vector<vtkm::Float64> positions;
  std::vector<vtkm::Float64> proportions;
  // major ticks
  CalculateTicks(this->Range, false, positions, proportions, this->MoreOrLessTickAdjustment);
  unsigned int nmajor = (unsigned int)proportions.size();
  while (this->Labels.size() < nmajor)
  {
    this->Labels.push_back(new TextAnnotationBillboard("test",
                                                       this->Color,
                                                       vtkm::Float32(this->FontScale),
                                                       vtkm::Vec<vtkm::Float32, 3>(0, 0, 0),
                                                       0));
  }

  std::stringstream numberToString;
  for (unsigned int i = 0; i < nmajor; ++i)
  {
    vtkm::Vec<vtkm::Float64, 3> tickPos =
      proportions[i] * (this->Point1 - this->Point0) + this->Point0;
    for (int pass = 0; pass <= 1; pass++)
    {
      vtkm::Vec<vtkm::Float64, 3> tickSize(0);
      if (pass == 0)
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[1] = this->TickMajorSize;
            break;
          case 1:
            tickSize[0] = this->TickMajorSize;
            break;
          case 2:
            tickSize[0] = this->TickMajorSize;
            break;
        }
      }
      else // pass == 1
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[2] = this->TickMajorSize;
            break;
          case 1:
            tickSize[2] = this->TickMajorSize;
            break;
          case 2:
            tickSize[1] = this->TickMajorSize;
            break;
        }
      }
      tickSize = tickSize * this->Invert;
      vtkm::Vec<vtkm::Float64, 3> start = tickPos - tickSize * this->TickMajorOffset;
      vtkm::Vec<vtkm::Float64, 3> end = tickPos - tickSize * (1.0 - this->TickMajorOffset);

      worldAnnotator.AddLine(start, end, this->LineWidth, this->Color, infront);
    }

    vtkm::Vec<vtkm::Float32, 3> tickSize(0);
    vtkm::Float32 s = 0.4f * this->FontOffset;
    switch (this->Axis)
    {
      case 0:
        tickSize[1] = s;
        tickSize[2] = s;
        break;
      case 1:
        tickSize[0] = s;
        tickSize[2] = s;
        break;
      case 2:
        tickSize[0] = s;
        tickSize[1] = s;
        break;
    }
    tickSize = tickSize * this->Invert;

    numberToString.str("");
    numberToString << positions[i];
    this->Labels[i]->SetText(numberToString.str());
    //if (fabs(positions[i]) < 1e-10)
    //    this->Labels[i]->SetText("0");
    this->Labels[i]->SetPosition(vtkm::Float32(tickPos[0] - tickSize[0]),
                                 vtkm::Float32(tickPos[1] - tickSize[1]),
                                 vtkm::Float32(tickPos[2] - tickSize[2]));
    this->Labels[i]->SetAlignment(TextAnnotation::HCenter, TextAnnotation::VCenter);
  }

  // minor ticks
  CalculateTicks(this->Range, true, positions, proportions, this->MoreOrLessTickAdjustment);
  unsigned int nminor = (unsigned int)proportions.size();
  for (unsigned int i = 0; i < nminor; ++i)
  {
    vtkm::Vec<vtkm::Float64, 3> tickPos =
      proportions[i] * (this->Point1 - this->Point0) + this->Point0;
    for (int pass = 0; pass <= 1; pass++)
    {
      vtkm::Vec<vtkm::Float64, 3> tickSize(0);
      if (pass == 0)
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[1] = this->TickMinorSize;
            break;
          case 1:
            tickSize[0] = this->TickMinorSize;
            break;
          case 2:
            tickSize[0] = this->TickMinorSize;
            break;
        }
      }
      else // pass == 1
      {
        switch (this->Axis)
        {
          case 0:
            tickSize[2] = this->TickMinorSize;
            break;
          case 1:
            tickSize[2] = this->TickMinorSize;
            break;
          case 2:
            tickSize[1] = this->TickMinorSize;
            break;
        }
      }
      tickSize = tickSize * this->Invert;
      vtkm::Vec<vtkm::Float64, 3> start = tickPos - tickSize * this->TickMinorOffset;
      vtkm::Vec<vtkm::Float64, 3> end = tickPos - tickSize * (1.0 - this->TickMinorOffset);

      worldAnnotator.AddLine(start, end, this->LineWidth, this->Color, infront);
    }
  }

  for (unsigned int i = 0; i < nmajor; ++i)
  {
    this->Labels[i]->Render(camera, worldAnnotator, canvas);
  }
}
}
} // namespace vtkm::rendering
