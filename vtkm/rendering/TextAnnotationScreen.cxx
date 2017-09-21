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

#include <vtkm/rendering/TextAnnotationScreen.h>

namespace vtkm
{
namespace rendering
{

TextAnnotationScreen::TextAnnotationScreen(const std::string& text,
                                           const vtkm::rendering::Color& color,
                                           vtkm::Float32 scale,
                                           const vtkm::Vec<vtkm::Float32, 2>& position,
                                           vtkm::Float32 angleDegrees)
  : TextAnnotation(text, color, scale)
  , Position(position)
  , Angle(angleDegrees)
{
}

TextAnnotationScreen::~TextAnnotationScreen()
{
}

void TextAnnotationScreen::SetPosition(const vtkm::Vec<vtkm::Float32, 2>& position)
{
  this->Position = position;
}

void TextAnnotationScreen::SetPosition(vtkm::Float32 xpos, vtkm::Float32 ypos)
{
  this->SetPosition(vtkm::make_Vec(xpos, ypos));
}

void TextAnnotationScreen::Render(const vtkm::rendering::Camera& vtkmNotUsed(camera),
                                  const vtkm::rendering::WorldAnnotator& vtkmNotUsed(annotator),
                                  vtkm::rendering::Canvas& canvas) const
{
  vtkm::Float32 windowAspect = vtkm::Float32(canvas.GetWidth()) / vtkm::Float32(canvas.GetHeight());

  canvas.AddText(this->Position,
                 this->Scale,
                 this->Angle,
                 windowAspect,
                 this->Anchor,
                 this->TextColor,
                 this->Text);
}
}
} // namespace vtkm::rendering
