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
#include <vtkm/io/ImageReaderPNG.h>
#include <vtkm/io/ImageReaderPNM.h>
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
  }
}

template <typename PixelType>
void TestCreateImageDataSet(const vtkm::rendering::Canvas& canvas)
{
  auto dataSet = canvas.GetDataSet("pixel-color");
  TestFilledImage<PixelType>(dataSet, "pixel-color", canvas);
}

template <typename PixelType>
void TestReadAndWritePNG(const vtkm::rendering::Canvas& canvas, std::string filename)
{
  auto pngWriter = PNGWriter();
  vtkm::cont::DataSet dataSet;
  bool throws = false;
  try
  {
    pngWriter.WriteToFile(filename, dataSet);
  }
  catch (const vtkm::cont::ErrorBadValue&)
  {
    throws = true;
  }
  VTKM_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  dataSet = canvas.GetDataSet(pngWriter.GetPointFieldName());
  pngWriter.WriteToFile(filename, dataSet);
  {
    vtkm::io::ImageReaderPNG reader(filename);
    dataSet = reader.ReadDataSet();
    // TODO: Fix this
    vtkm::cont::Field field = dataSet.GetField(reader.GetPointFieldName());
    dataSet.AddPointField(pngWriter.GetPointFieldName(), field.GetData());
  }
  pngWriter.WriteToFile(filename, dataSet);
  {
    vtkm::io::ImageReaderPNG reader(filename);
    dataSet = reader.ReadDataSet();
    TestFilledImage<PixelType>(dataSet, reader.GetPointFieldName(), canvas);
  }
}

template <const vtkm::Id BitDepth>
void TestReadAndWritePNM(const vtkm::rendering::Canvas& canvas)
{
  using PixelType = RGBPixel<BitDepth>;
  PNMWriter ppmWriter((1 << BitDepth) - 1);
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

  dataSet = canvas.GetDataSet(ppmWriter.GetPointFieldName());
  ppmWriter.WriteToFile("ppmTestFile.ppm", dataSet);
  {
    vtkm::io::ImageReaderPNM reader("ppmTestFile.ppm");
    dataSet = reader.ReadDataSet();
    // TODO: Fix this
    vtkm::cont::Field field = dataSet.GetField(reader.GetPointFieldName());
    dataSet.AddPointField(ppmWriter.GetPointFieldName(), field.GetData());
  }
  ppmWriter.WriteToFile("ppmTestFile2.ppm", dataSet);
  {
    vtkm::io::ImageReaderPNM reader("ppmTestFile2.ppm");
    dataSet = reader.ReadDataSet();
    TestFilledImage<PixelType>(dataSet, reader.GetPointFieldName(), canvas);
  }
}


void TestBaseImageMethods(const vtkm::rendering::Canvas& canvas)
{
  TestCreateImageDataSet<RGBPixel_8>(canvas);
  TestCreateImageDataSet<RGBPixel_16>(canvas);
  TestCreateImageDataSet<GreyPixel_8>(canvas);
  TestCreateImageDataSet<GreyPixel_16>(canvas);
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
