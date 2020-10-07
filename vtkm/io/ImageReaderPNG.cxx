//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/ImageReaderPNG.h>

#include <vtkm/io/PixelTypes.h>

namespace
{

VTKM_CONT
template <typename PixelType>
vtkm::io::ImageReaderBase::ColorArrayType ReadFromPNG(const std::string& fileName,
                                                      vtkm::Id& width,
                                                      vtkm::Id& height)
{
  unsigned char* imageData;
  unsigned uwidth, uheight;
  vtkm::png::lodepng_decode_file(&imageData,
                                 &uwidth,
                                 &uheight,
                                 fileName.c_str(),
                                 PixelType::PNG_COLOR_TYPE,
                                 PixelType::BIT_DEPTH);

  width = static_cast<vtkm::Id>(uwidth);
  height = static_cast<vtkm::Id>(uheight);

  // Fill in the data starting from the end (Images are read Top-Left to Bottom-Right,
  // but are stored from Bottom-Left to Top-Right)
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> array;
  array.Allocate(width * height);
  auto portal = array.WritePortal();
  vtkm::Id vtkmIndex = 0;
  for (vtkm::Id yIndex = static_cast<vtkm::Id>(height - 1); yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < static_cast<vtkm::Id>(width); xIndex++)
    {
      vtkm::Id pngIndex = static_cast<vtkm::Id>(yIndex * width + xIndex);
      portal.Set(vtkmIndex, PixelType(imageData, pngIndex).ToVec4f());
      vtkmIndex++;
    }
  }

  free(imageData);
  return array;
}

} // anonymous namespace

namespace vtkm
{
namespace io
{

ImageReaderPNG::~ImageReaderPNG() noexcept {}

void ImageReaderPNG::Read()
{
  vtkm::Id width;
  vtkm::Id height;
  vtkm::io::ImageReaderBase::ColorArrayType pixelArray =
    ReadFromPNG<vtkm::io::RGBPixel_16>(this->FileName, width, height);

  this->InitializeImageDataSet(width, height, pixelArray);
}
}
} // namespace vtkm::io
