//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/ImageReaderPNM.h>

#include <vtkm/io/PixelTypes.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace io
{

ImageReaderPNM::~ImageReaderPNM() noexcept {}

VTKM_CONT
void ImageReaderPNM::Read()
{
  std::ifstream inStream(this->FileName.c_str(), std::ios_base::binary | std::ios_base::in);

  // Currently, the only magic number supported is P6
  std::string magicNum;
  inStream >> magicNum;
  if (magicNum != "P6")
  {
    throw vtkm::cont::ErrorBadValue("MagicNumber: " + magicNum + " in file: " + this->FileName +
                                    " did not match: P6");
  }

  vtkm::Id width;
  vtkm::Id height;
  vtkm::Id maxColorValue;
  inStream >> width >> height >> maxColorValue;
  inStream.get();

  if ((maxColorValue > 0) && (maxColorValue <= 255))
  {
    this->DecodeFile<vtkm::io::RGBPixel_8>(inStream, width, height);
  }
  else if ((maxColorValue > 255) && (maxColorValue <= 65535))
  {
    this->DecodeFile<vtkm::io::RGBPixel_16>(inStream, width, height);
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("MaxColorValue: " + std::to_string(maxColorValue) +
                                    " from file: " + this->FileName +
                                    " is not in valid range of [1, 65535]");
  }
}

VTKM_CONT
template <typename PixelType>
void ImageReaderPNM::DecodeFile(std::ifstream& inStream,
                                const vtkm::Id& width,
                                const vtkm::Id& height)
{
  vtkm::UInt32 imageSize = static_cast<vtkm::UInt32>(width * height * PixelType::BYTES_PER_PIXEL);
  std::vector<unsigned char> imageData(imageSize);
  inStream.read(reinterpret_cast<char*>(imageData.data()), imageSize);

  // Fill in the data starting from the end (Images are read Top-Left to Bottom-Right,
  // but are stored from Bottom-Left to Top-Right)
  vtkm::io::ImageReaderBase::ColorArrayType array;
  array.Allocate(width * height);
  auto portal = array.WritePortal();
  vtkm::Id vtkmIndex = 0;
  for (vtkm::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < width; xIndex++)
    {
      vtkm::Id pnmIndex = yIndex * width + xIndex;
      portal.Set(vtkmIndex, PixelType(imageData.data(), pnmIndex).ToVec4f());
      vtkmIndex++;
    }
  }

  this->InitializeImageDataSet(width, height, array);
}
}
} // namespace vtkm::io
