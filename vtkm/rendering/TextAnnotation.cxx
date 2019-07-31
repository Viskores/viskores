//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/TextAnnotation.h>

namespace vtkm
{
namespace rendering
{

TextAnnotation::TextAnnotation(const std::string& text,
                               const vtkm::rendering::Color& color,
                               vtkm::Float32 scale)
  : Text(text)
  , TextColor(color)
  , Scale(scale)
  , Anchor(-1, -1)
{
}

TextAnnotation::~TextAnnotation()
{
}

void TextAnnotation::SetText(const std::string& text)
{
  this->Text = text;
}

const std::string& TextAnnotation::GetText() const
{
  return this->Text;
}

void TextAnnotation::SetRawAnchor(const vtkm::Vec2f_32& anchor)
{
  this->Anchor = anchor;
}

void TextAnnotation::SetRawAnchor(vtkm::Float32 h, vtkm::Float32 v)
{
  this->SetRawAnchor(vtkm::make_Vec(h, v));
}

void TextAnnotation::SetAlignment(HorizontalAlignment h, VerticalAlignment v)
{
  switch (h)
  {
    case Left:
      this->Anchor[0] = -1.0f;
      break;
    case HCenter:
      this->Anchor[0] = 0.0f;
      break;
    case Right:
      this->Anchor[0] = +1.0f;
      break;
  }

  // For vertical alignment, "center" is generally the center
  // of only the above-baseline contents of the font, so we
  // use a value slightly off of zero for VCenter.
  // (We don't use an offset value instead of -1.0 for the
  // bottom value, because generally we want a true minimum
  // extent, e.g. to have text sitting at the bottom of a
  // window, and in that case, we need to keep all the text,
  // including parts that descend below the baseline, above
  // the bottom of the window.
  switch (v)
  {
    case Bottom:
      this->Anchor[1] = -1.0f;
      break;
    case VCenter:
      this->Anchor[1] = -0.06f;
      break;
    case Top:
      this->Anchor[1] = +1.0f;
      break;
  }
}

void TextAnnotation::SetScale(vtkm::Float32 scale)
{
  this->Scale = scale;
}
}
} // namespace vtkm::rendering
