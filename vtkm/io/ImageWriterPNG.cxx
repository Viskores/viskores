//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/ImageWriterPNG.h>

#include <vtkm/io/PixelTypes.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace io
{

ImageWriterPNG::~ImageWriterPNG() noexcept {}

void ImageWriterPNG::Write(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels)
{
  switch (this->Depth)
  {
    case PixelDepth::PIXEL_8:
      this->WriteToFile<vtkm::io::RGBPixel_8>(width, height, pixels);
      break;
    case PixelDepth::PIXEL_16:
      WriteToFile<vtkm::io::RGBPixel_16>(width, height, pixels);
      break;
  }
}

template <typename PixelType>
void ImageWriterPNG::WriteToFile(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels)
{
  auto pixelPortal = pixels.ReadPortal();
  std::vector<unsigned char> imageData(static_cast<typename std::vector<unsigned char>::size_type>(
    pixels.GetNumberOfValues() * PixelType::BYTES_PER_PIXEL));

  // Write out the data starting from the end (Images are stored Bottom-Left to Top-Right,
  // but are viewed from Top-Left to Bottom-Right)
  vtkm::Id pngIndex = 0;
  for (vtkm::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < width; xIndex++)
    {
      vtkm::Id vtkmIndex = yIndex * width + xIndex;
      PixelType(pixelPortal.Get(vtkmIndex)).FillImageAtIndexWithPixel(imageData.data(), pngIndex);
      pngIndex++;
    }
  }

  vtkm::png::lodepng_encode_file(
    this->FileName.c_str(),
    imageData.data(),
    static_cast<unsigned>(width),
    static_cast<unsigned>(height),
    static_cast<vtkm::png::LodePNGColorType>(PixelType::GetColorType()),
    PixelType::GetBitDepth());
}
}
} // namespace vtkm::io
