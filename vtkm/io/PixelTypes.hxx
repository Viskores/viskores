//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_PixelTypes_hxx
#define vtk_m_io_PixelTypes_hxx

#include <vtkm/Math.h>
#include <vtkm/io/PixelTypes.h>

namespace vtkm
{
namespace io
{

template <const vtkm::Id B, const vtkm::IdComponent C>
void BasePixel<B, C>::FillImageAtIndexWithPixel(unsigned char* imageData, const vtkm::Id index)
{
  vtkm::Id initShift = BIT_DEPTH - 8;
  for (vtkm::Id channel = 0; channel < NUM_CHANNELS; channel++)
  {
    for (vtkm::Id shift = initShift, i = 0; shift >= 0; shift -= 8, i++)
    {
      imageData[index * BYTES_PER_PIXEL + i + (channel * NUM_BYTES)] =
        static_cast<unsigned char>((this->Components[channel] & (0xff << shift)) >> shift);
    }
  }
}

template <const vtkm::Id B, const vtkm::IdComponent C>
void BasePixel<B, C>::ConstructPixelFromImage(const unsigned char* imageData, const vtkm::Id index)
{
  vtkm::Id initShift = BIT_DEPTH - 8;
  for (vtkm::Id channel = 0; channel < NUM_CHANNELS; channel++)
  {
    for (vtkm::Id shift = initShift, i = 0; shift >= 0; shift -= 8, i++)
    {
      this->Components[channel] |= imageData[index * BYTES_PER_PIXEL + i + (channel * NUM_BYTES)]
        << shift;
    }
  }
}

template <const vtkm::Id B>
typename RGBPixel<B>::ComponentType RGBPixel<B>::Diff(const Superclass& pixel) const
{
  return static_cast<RGBPixel<B>::ComponentType>(vtkm::Abs(this->Components[0] - pixel[0]) +
                                                 vtkm::Abs(this->Components[1] - pixel[1]) +
                                                 vtkm::Abs(this->Components[2] - pixel[2]));
}

template <const vtkm::Id B>
vtkm::Vec4f_32 RGBPixel<B>::ToVec4f() const
{
  return vtkm::Vec4f_32(static_cast<vtkm::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
                        static_cast<vtkm::Float32>(this->Components[1]) / this->MAX_COLOR_VALUE,
                        static_cast<vtkm::Float32>(this->Components[2]) / this->MAX_COLOR_VALUE,
                        1);
}

template <const vtkm::Id B>
typename GreyPixel<B>::ComponentType GreyPixel<B>::Diff(const Superclass& pixel) const
{
  return static_cast<GreyPixel<B>::ComponentType>(vtkm::Abs(this->Components[0] - pixel[0]));
}

template <const vtkm::Id B>
vtkm::Vec4f_32 GreyPixel<B>::ToVec4f() const
{
  return vtkm::Vec4f_32(static_cast<vtkm::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
                        static_cast<vtkm::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
                        static_cast<vtkm::Float32>(this->Components[0]) / this->MAX_COLOR_VALUE,
                        1);
}

template <const vtkm::Id B>
int RGBPixel<B>::GetColorType()
{
  return internal::RGBColorType;
}

template <const vtkm::Id B>
int GreyPixel<B>::GetColorType()
{
  return internal::GreyColorType;
}

} // namespace io
} // namespace vtkm

#endif //vtk_m_io_PixelTypes_h
