//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/BitmapFont.h>

namespace vtkm
{
namespace rendering
{

BitmapFont::BitmapFont()
{
  for (int i = 0; i < 256; ++i)
    ShortMap[i] = 0;
  this->PadL = 0;
  this->PadR = 0;
  this->PadT = 0;
  this->PadB = 0;
}

const vtkm::rendering::BitmapFont::Character& BitmapFont::GetChar(char c) const
{
  std::size_t mappedCharIndex = static_cast<std::size_t>(this->ShortMap[(unsigned char)c]);
  return this->Chars[mappedCharIndex];
}

vtkm::Float32 BitmapFont::GetTextWidth(const std::string& text) const
{
  vtkm::Float32 width = 0;
  for (unsigned int i = 0; i < text.length(); ++i)
  {
    Character c = this->GetChar(text[i]);
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;

    const bool kerning = true;
    if (kerning && nextchar > 0)
      width += vtkm::Float32(c.kern[int(nextchar)]) / vtkm::Float32(this->Height);
    width += vtkm::Float32(c.adv) / vtkm::Float32(this->Height);
  }
  return width;
}

void BitmapFont::GetCharPolygon(char character,
                                vtkm::Float32& x,
                                vtkm::Float32& y,
                                vtkm::Float32& vl,
                                vtkm::Float32& vr,
                                vtkm::Float32& vt,
                                vtkm::Float32& vb,
                                vtkm::Float32& tl,
                                vtkm::Float32& tr,
                                vtkm::Float32& tt,
                                vtkm::Float32& tb,
                                char nextchar) const
{
  Character c = this->GetChar(character);

  // By default, the origin for the font is at the
  // baseline.  That's nice, but we'd rather it
  // be at the actual bottom, so create an offset.
  vtkm::Float32 yoff = -vtkm::Float32(this->Descender) / vtkm::Float32(this->Height);

  tl = vtkm::Float32(c.x + this->PadL) / vtkm::Float32(this->ImgW);
  tr = vtkm::Float32(c.x + c.w - this->PadR) / vtkm::Float32(this->ImgW);
  tt = 1.f - vtkm::Float32(c.y + this->PadT) / vtkm::Float32(this->ImgH);
  tb = 1.f - vtkm::Float32(c.y + c.h - this->PadB) / vtkm::Float32(this->ImgH);

  vl = x + vtkm::Float32(c.offx + this->PadL) / vtkm::Float32(this->Height);
  vr = x + vtkm::Float32(c.offx + c.w - this->PadR) / vtkm::Float32(this->Height);
  vt = yoff + y + vtkm::Float32(c.offy - this->PadT) / vtkm::Float32(this->Height);
  vb = yoff + y + vtkm::Float32(c.offy - c.h + this->PadB) / vtkm::Float32(this->Height);

  const bool kerning = true;
  if (kerning && nextchar > 0)
    x += vtkm::Float32(c.kern[int(nextchar)]) / vtkm::Float32(this->Height);
  x += vtkm::Float32(c.adv) / vtkm::Float32(this->Height);
}
}
} // namespace vtkm::rendering
