//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/ImageWriterPNM.h>

#include <vtkm/io/PixelTypes.h>

namespace vtkm
{
namespace io
{

ImageWriterPNM::~ImageWriterPNM() noexcept {}

void ImageWriterPNM::Write(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels)
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
void ImageWriterPNM::WriteToFile(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels)
{
  std::ofstream outStream(this->FileName.c_str(), std::ios_base::binary | std::ios_base::out);
  outStream << "P6\n" << width << " " << height << "\n";

  outStream << PixelType::MAX_COLOR_VALUE << "\n";
  auto pixelPortal = pixels.ReadPortal();

  vtkm::UInt32 imageSize =
    static_cast<vtkm::UInt32>(pixels.GetNumberOfValues() * PixelType::BYTES_PER_PIXEL);
  std::vector<unsigned char> imageData(imageSize);

  // Write out the data starting from the end (Images are stored Bottom-Left to Top-Right,
  // but are viewed from Top-Left to Bottom-Right)
  vtkm::Id pnmIndex = 0;
  for (vtkm::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < width; xIndex++, pnmIndex++)
    {
      vtkm::Id vtkmIndex = yIndex * width + xIndex;
      PixelType(pixelPortal.Get(vtkmIndex)).FillImageAtIndexWithPixel(imageData.data(), pnmIndex);
    }
  }
  outStream.write((char*)imageData.data(), imageSize);
  outStream.close();
}
}
} // namespace vtkm::io
