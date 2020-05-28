//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReader_hxx
#define vtk_m_io_ImageReader_hxx

#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/io/ImageReader.h>
#include <vtkm/io/PixelTypes.h>
#include <vtkm/rendering/Canvas.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/lodepng/vtkmlodepng/lodepng.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace io
{

// Start BaseReaderImage Class Template Implementations
VTKM_CONT
vtkm::cont::DataSet BaseImageReader::CreateImageDataSet(const vtkm::rendering::Canvas& canvas)
{
  return this->CreateImageDataSet(canvas.GetColorBuffer(), canvas.GetWidth(), canvas.GetHeight());
}

VTKM_CONT
vtkm::cont::DataSet BaseImageReader::CreateImageDataSet(
  const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorBuffer,
  const vtkm::Id& width,
  const vtkm::Id& height)
{
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32>::ReadPortalType colorPortal = colorBuffer.ReadPortal();
  std::vector<vtkm::Vec4f_32> fieldData;
  for (vtkm::Id yIndex = 0; yIndex < height; yIndex++)
  {
    for (vtkm::Id xIndex = 0; xIndex < width; xIndex++)
    {
      vtkm::Vec4f_32 tuple = colorPortal.Get(yIndex * width + xIndex);
      fieldData.push_back(tuple);
    }
  }
  auto dataSet = this->InitializeImageDataSet(width, height);
  dataSet.AddPointField(this->PointFieldName, fieldData);
  return dataSet;
}

VTKM_CONT
vtkm::cont::DataSet BaseImageReader::InitializeImageDataSet(const vtkm::Id& width,
                                                            const vtkm::Id& height)
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(width, height);
  return dsb.Create(dimensions);
}
// End BaseReaderImage Class Template Implementations

// Start PNGReader Class Template Implementations
VTKM_CONT
vtkm::cont::DataSet PNGReader::ReadFromFile(const std::string& fileName)
{
  return this->ReadFromFile<io::RGBPixel_16>(fileName);
}

VTKM_CONT
template <typename PixelType>
vtkm::cont::DataSet PNGReader::ReadFromFile(const std::string& fileName)
{
  unsigned char* imageData;
  unsigned uwidth, uheight;
  vtkm::Id width, height;
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
  std::vector<vtkm::Vec4f_32> fieldData;
  for (vtkm::Id yIndex = static_cast<vtkm::Id>(height - 1); yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < static_cast<vtkm::Id>(width); xIndex++)
    {
      vtkm::Id index = static_cast<vtkm::Id>(yIndex * width + xIndex);
      fieldData.push_back(PixelType(imageData, index).ToVec4f());
    }
  }

  auto dataSet = this->InitializeImageDataSet(width, height);
  dataSet.AddPointField(this->PointFieldName, fieldData);

  free(imageData);
  return dataSet;
}
// End PNGReader Class Template Implementations

// Start PNMReader Class Template Implementations
VTKM_CONT
vtkm::cont::DataSet PNMReader::ReadFromFile(const std::string& fileName)
{
  std::ifstream inStream(fileName.c_str(), std::ios_base::binary | std::ios_base::in);
  vtkm::Id width;
  vtkm::Id height;
  std::string val;

  inStream >> val;
  if (this->MagicNumber != val)
  {
    throw vtkm::cont::ErrorBadValue("MagicNumber: " + this->MagicNumber + " in file: " + fileName +
                                    " did not match: " + val);
  }

  inStream >> width >> height >> this->MaxColorValue;
  inStream.get();

  switch (this->MaxColorValue)
  {
    case 255:
      return this->DecodeFile<io::RGBPixel_8>(inStream, width, height);
    case 65535:
      return this->DecodeFile<io::RGBPixel_16>(inStream, width, height);
    default:
      throw vtkm::cont::ErrorBadValue("MaxColorValue: " + std::to_string(this->MaxColorValue) +
                                      " from file: " + fileName + " was not one of: {8, 16}");
  }
}

VTKM_CONT
template <typename PixelType>
vtkm::cont::DataSet PNMReader::DecodeFile(std::ifstream& inStream,
                                          const vtkm::Id& width,
                                          const vtkm::Id& height)
{
  vtkm::UInt32 imageSize = static_cast<vtkm::UInt32>(width * height * PixelType::BYTES_PER_PIXEL);
  std::vector<unsigned char> imageData(imageSize);
  inStream.read((char*)imageData.data(), imageSize);

  // Fill in the data starting from the end (Images are read Top-Left to Bottom-Right,
  // but are stored from Bottom-Left to Top-Right)
  std::vector<vtkm::Vec4f_32> fieldData;
  for (vtkm::Id yIndex = height - 1; yIndex >= 0; yIndex--)
  {
    for (vtkm::Id xIndex = 0; xIndex < width; xIndex++)
    {
      vtkm::Id index = yIndex * width + xIndex;
      fieldData.push_back(PixelType(imageData.data(), index).ToVec4f());
    }
  }

  auto dataSet = this->InitializeImageDataSet(width, height);
  dataSet.AddPointField(this->PointFieldName, fieldData);
  return dataSet;
}
// End PNMReader Class Template Implementations

} // namespace io
} // namespace vtkm

#endif
