//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageWriter_hxx
#define vtk_m_io_ImageWriter_hxx

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/io/ImageWriter.h>
#include <vtkm/io/PixelTypes.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace io
{

VTKM_CONT
vtkm::Id BaseImageWriter::GetImageWidth(vtkm::cont::DataSet dataSet) const
{
  if (dataSet.GetNumberOfCoordinateSystems() > 0)
  {
    // Add 1 since the Bounds are 0 indexed
    return static_cast<vtkm::Id>(dataSet.GetCoordinateSystem().GetBounds().X.Max) + 1;
  }
  return 0;
}

VTKM_CONT
vtkm::Id BaseImageWriter::GetImageHeight(vtkm::cont::DataSet dataSet) const
{
  if (dataSet.GetNumberOfCoordinateSystems() > 0)
  {
    // Add 1 since the Bounds are 0 indexed
    return static_cast<vtkm::Id>(dataSet.GetCoordinateSystem().GetBounds().Y.Max) + 1;
  }
  return 0;
}

VTKM_CONT
void PNMWriter::WriteToFile(const std::string& fileName, const vtkm::cont::DataSet& dataSet) const
{
  if (!dataSet.HasField(this->PointFieldName))
  {
    throw vtkm::cont::ErrorBadValue(
      "No pixel data found in DataSet, cannot write without image data!");
  }

  std::ofstream outStream(fileName.c_str(), std::ios_base::binary | std::ios_base::out);
  outStream << this->MagicNumber << std::endl
            << this->GetImageWidth(dataSet) << " " << this->GetImageHeight(dataSet) << std::endl;

  switch (this->MaxColorValue)
  {
    case 0:
      this->EncodeFile<RGBPixel_8>(outStream, dataSet);
      break;
    case 255:
      this->EncodeFile<RGBPixel_8>(outStream, dataSet);
      break;
    case 65535:
      this->EncodeFile<RGBPixel_16>(outStream, dataSet);
      break;
    default:
      throw vtkm::cont::ErrorBadValue("MaxColorValue: " + std::to_string(this->MaxColorValue) +
                                      " was not one of: {255, 65535}");
  }
}

VTKM_CONT
template <typename PixelType>
void PNMWriter::EncodeFile(std::ofstream& outStream, const vtkm::cont::DataSet& dataSet) const
{
  outStream << PixelType::MAX_COLOR_VALUE << std::endl;
  auto pixelField = dataSet.GetPointField(this->PointFieldName)
                      .GetData()
                      .template Cast<vtkm::cont::ArrayHandle<vtkm::Vec4f_32>>();
  auto pixelPortal = pixelField.ReadPortal();

  vtkm::UInt32 imageSize =
    static_cast<vtkm::UInt32>(pixelField.GetNumberOfValues() * PixelType::BYTES_PER_PIXEL);
  std::vector<unsigned char> imageData(imageSize);

  // Write out the data starting from the end (Images are stored Bottom-Left to Top-Right,
  // but are viewed from Top-Left to Bottom-Right)
  vtkm::Id imageIndex = 0;
  vtkm::Id imageHeight = this->GetImageHeight(dataSet);
  vtkm::Id imageWidth = this->GetImageWidth(dataSet);
  for (vtkm::Id yIndex = imageHeight - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < imageWidth; xIndex++, imageIndex++)
    {
      vtkm::Id index = yIndex * imageWidth + xIndex;
      PixelType(pixelPortal.Get(index)).FillImageAtIndexWithPixel(imageData.data(), imageIndex);
    }
  }
  outStream.write((char*)imageData.data(), imageSize);
  outStream.close();
}

VTKM_CONT
void PNGWriter::WriteToFile(const std::string& fileName, const vtkm::cont::DataSet& dataSet) const
{
  switch (this->MaxColorValue)
  {
    case 0:
      WriteToFile<RGBPixel_8>(fileName, dataSet);
      break;
    case 255:
      WriteToFile<RGBPixel_8>(fileName, dataSet);
      break;
    case 65535:
      WriteToFile<RGBPixel_16>(fileName, dataSet);
      break;
    default:
      throw vtkm::cont::ErrorBadValue("MaxColorValue: " + std::to_string(this->MaxColorValue) +
                                      " was not one of: {255, 65535}");
  }
}

VTKM_CONT
template <typename PixelType>
void PNGWriter::WriteToFile(const std::string& fileName, const vtkm::cont::DataSet& dataSet) const
{
  if (!dataSet.HasField(this->PointFieldName))
  {
    throw vtkm::cont::ErrorBadValue(
      "No pixel data found in DataSet, cannot write without image data!");
  }

  auto pixelField = dataSet.GetPointField(this->PointFieldName)
                      .GetData()
                      .template Cast<vtkm::cont::ArrayHandle<vtkm::Vec4f_32>>();
  auto pixelPortal = pixelField.ReadPortal();
  std::vector<unsigned char> imageData(static_cast<typename std::vector<unsigned char>::size_type>(
    pixelField.GetNumberOfValues() * PixelType::BYTES_PER_PIXEL));

  // Write out the data starting from the end (Images are stored Bottom-Left to Top-Right,
  // but are viewed from Top-Left to Bottom-Right)
  vtkm::Id imageIndex = 0;
  vtkm::Id imageHeight = this->GetImageHeight(dataSet);
  vtkm::Id imageWidth = this->GetImageWidth(dataSet);
  for (vtkm::Id yIndex = imageHeight - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < imageWidth; xIndex++, imageIndex++)
    {
      vtkm::Id index = yIndex * imageWidth + xIndex;
      PixelType(pixelPortal.Get(index)).FillImageAtIndexWithPixel(imageData.data(), imageIndex);
    }
  }

  vtkm::png::lodepng_encode_file(fileName.c_str(),
                                 imageData.data(),
                                 static_cast<unsigned>(imageWidth),
                                 static_cast<unsigned>(imageHeight),
                                 PixelType::PNG_COLOR_TYPE,
                                 PixelType::BIT_DEPTH);
}

} // namespace io
} // namespace vtkm

#endif
