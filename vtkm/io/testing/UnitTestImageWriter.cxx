//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/ImageReader.h>
#include <vtkm/io/ImageWriter.h>
#include <vtkm/io/PixelTypes.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>

#include <string>

namespace
{

using namespace vtkm::io;
using namespace vtkm::rendering;

template <typename PixelType>
void TestFilledImage(vtkm::cont::DataSet& dataSet,
                     const std::string& fieldName,
                     const vtkm::rendering::Canvas& canvas)
{
  VTKM_TEST_ASSERT(dataSet.HasPointField(fieldName), "Point Field Not Found: " + fieldName);

  auto pointField = dataSet.GetPointField(fieldName);
  VTKM_TEST_ASSERT(pointField.GetNumberOfValues() == canvas.GetWidth() * canvas.GetHeight(),
                   "wrong image dimensions");
  VTKM_TEST_ASSERT(pointField.GetData().template IsType<vtkm::cont::ArrayHandle<vtkm::Vec4f_32>>(),
                   "wrong ArrayHandle type");
  auto pixelPortal =
    pointField.GetData().template Cast<vtkm::cont::ArrayHandle<vtkm::Vec4f_32>>().ReadPortal();

  auto colorPortal = canvas.GetColorBuffer().ReadPortal();
  for (vtkm::Id y = 0; y < canvas.GetHeight(); y++)
  {
    std::ostringstream row;
    row << "[";
    for (vtkm::Id x = 0; x < canvas.GetWidth(); x++)
    {
      auto tuple = colorPortal.Get(y * canvas.GetWidth() + x);
      auto pixelVec = PixelType(pixelPortal.Get(y * canvas.GetWidth() + x));
      std::ostringstream data;
      data << pixelVec << ":" << PixelType(tuple) << std::endl;
      VTKM_TEST_ASSERT(pixelVec == PixelType(tuple),
                       "Stored pixel did not match canvas value" + data.str());
      row << pixelVec << ",";
    }
    row << "]";
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Row[" << y << "]" << row.str());
  }
}

template <typename PixelType>
void TestCreateImageDataSet(BaseImageReader& reader, const vtkm::rendering::Canvas& canvas)
{
  auto dataSet = reader.CreateImageDataSet(canvas);
  TestFilledImage<PixelType>(dataSet, reader.GetPointFieldName(), canvas);
}

template <typename PixelType>
void TestReadAndWritePNG(const vtkm::rendering::Canvas& canvas, std::string image)
{
  auto pngReader = PNGReader();
  auto pngWriter = PNGWriter();
  vtkm::cont::DataSet dataSet;
  bool throws = false;
  try
  {
    pngWriter.WriteToFile(image, dataSet);
  }
  catch (const vtkm::cont::ErrorBadValue&)
  {
    throws = true;
  }
  VTKM_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  dataSet = pngReader.CreateImageDataSet(canvas);
  pngWriter.WriteToFile(image, dataSet);
  dataSet = pngReader.ReadFromFile(image);
  pngWriter.WriteToFile(image, dataSet);
  dataSet = pngReader.ReadFromFile(image);
  TestFilledImage<PixelType>(dataSet, pngReader.GetPointFieldName(), canvas);
}

template <const vtkm::Id BitDepth>
void TestReadAndWritePNM(const vtkm::rendering::Canvas& canvas)
{
  using PixelType = RGBPixel<BitDepth>;
  PNMWriter ppmWriter((1 << BitDepth) - 1);
  PNMReader ppmReader((1 << BitDepth) - 1);
  vtkm::cont::DataSet dataSet;
  bool throws = false;
  try
  {
    ppmWriter.WriteToFile("ppmTestFile" + std::to_string(BitDepth) + "bit.ppm", dataSet);
  }
  catch (const vtkm::cont::ErrorBadValue&)
  {
    throws = true;
  }
  VTKM_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  dataSet = ppmReader.CreateImageDataSet(canvas);
  ppmWriter.WriteToFile("ppmTestFile.ppm", dataSet);
  dataSet = ppmReader.ReadFromFile("ppmTestFile.ppm");
  ppmWriter.WriteToFile("ppmTestFile2.ppm", dataSet);
  dataSet = ppmReader.ReadFromFile("ppmTestFile2.ppm");
  TestFilledImage<PixelType>(dataSet, ppmReader.GetPointFieldName(), canvas);
}


void TestBaseImageMethods(const vtkm::rendering::Canvas& canvas)
{
  auto reader = PNGReader();
  TestCreateImageDataSet<RGBPixel_8>(reader, canvas);
  TestCreateImageDataSet<RGBPixel_16>(reader, canvas);
  TestCreateImageDataSet<GreyPixel_8>(reader, canvas);
  TestCreateImageDataSet<GreyPixel_16>(reader, canvas);
}

void TestPNMImage(const vtkm::rendering::Canvas& canvas)
{
  TestReadAndWritePNM<8>(canvas);
  TestReadAndWritePNM<16>(canvas);
}

void TestPNGImage(const vtkm::rendering::Canvas& canvas)
{
  TestReadAndWritePNG<RGBPixel_8>(canvas, "pngRGB8Test.png");
  TestReadAndWritePNG<RGBPixel_16>(canvas, "pngRGB16Test.png");
  TestReadAndWritePNG<GreyPixel_8>(canvas, "pngGrey8Test.png");
  TestReadAndWritePNG<GreyPixel_16>(canvas, "pngGrey16Test.png");
}

void TestImage()
{
  vtkm::rendering::Canvas canvas(16, 16);
  canvas.SetBackgroundColor(vtkm::rendering::Color::red);
  canvas.Initialize();
  canvas.Activate();
  canvas.Clear();
  // Line from top left to bottom right, ensures correct transposedness
  canvas.AddLine(-0.9, 0.9, 0.9, -0.9, 2.0f, vtkm::rendering::Color::black);
  vtkm::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  canvas.AddColorBar(colorBarBounds, vtkm::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  canvas.SaveAs("baseline.ppm");

  TestBaseImageMethods(canvas);
  TestPNMImage(canvas);
  TestPNGImage(canvas);
}
}

int UnitTestImageWriter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestImage, argc, argv);
}
