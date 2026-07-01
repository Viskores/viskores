//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/ImageReaderPNG.h>
#include <viskores/io/ImageReaderPNM.h>
#include <viskores/io/ImageWriterBase.h>
#include <viskores/io/ImageWriterPNG.h>
#include <viskores/io/ImageWriterPNM.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>

#include <string>

namespace
{

using namespace viskores::io;
using namespace viskores::rendering;

constexpr viskores::IdComponent ImageWidth = 2;
constexpr viskores::IdComponent ImageHeight = 2;
const std::string ColorFieldName = "color";

viskores::Vec4f_32 ExpectedRGB16(viskores::Float32 red,
                                 viskores::Float32 green,
                                 viskores::Float32 blue)
{
  auto convert = [](viskores::Float32 channel)
  {
    constexpr viskores::Float32 MaxChannelValue = 65535.0f;
    return static_cast<viskores::Float32>(static_cast<viskores::UInt16>(channel * MaxChannelValue) /
                                          MaxChannelValue);
  };
  return viskores::Vec4f_32(convert(red), convert(green), convert(blue), 1.0f);
}

template <typename ArrayHandleType>
viskores::cont::DataSet MakeImageDataSet(const ArrayHandleType& colorArray)
{
  viskores::cont::DataSet dataSet =
    viskores::cont::DataSetBuilderUniform::Create(viskores::Id2(ImageWidth, ImageHeight));
  dataSet.AddPointField(ColorFieldName, colorArray);
  return dataSet;
}

void ValidateWrittenImage(const viskores::cont::DataSet& dataSet,
                          const std::string& fieldName,
                          const viskores::io::ImageWriterBase::ColorArrayType& expectedPixels)
{
  VISKORES_TEST_ASSERT(dataSet.HasPointField(fieldName), "Point Field Not Found: " + fieldName);

  auto pointField = dataSet.GetPointField(fieldName);
  VISKORES_TEST_ASSERT(pointField.GetNumberOfValues() == ImageWidth * ImageHeight,
                       "wrong image dimensions");
  VISKORES_TEST_ASSERT(pointField.GetNumberOfValues() == expectedPixels.GetNumberOfValues(),
                       "wrong number of image pixels");
  VISKORES_TEST_ASSERT(
    pointField.GetData().template IsType<viskores::cont::ArrayHandle<viskores::Vec4f_32>>(),
    "wrong ArrayHandle type");

  auto pixels =
    pointField.GetData().template AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec4f_32>>();
  auto pixelCompare = test_equal_ArrayHandles(pixels, expectedPixels);
  VISKORES_TEST_ASSERT(pixelCompare, pixelCompare.GetMergedMessage());
}

template <typename WriterType, typename ReaderType, typename ArrayHandleType>
void TestWriteAndReadColorArray(const std::string& filename,
                                const ArrayHandleType& colorArray,
                                const viskores::io::ImageWriterBase::ColorArrayType& expectedPixels)
{
  {
    WriterType writer(filename);
    writer.SetPixelDepth(viskores::io::ImageWriterBase::PixelDepth::PIXEL_16);
    writer.WriteDataSet(MakeImageDataSet(colorArray), ColorFieldName);
  }

  ReaderType reader(filename);
  viskores::cont::DataSet readDataSet = reader.ReadDataSet();
  ValidateWrittenImage(readDataSet, reader.GetPointFieldName(), expectedPixels);
}

void TestFilledImage(viskores::cont::DataSet& dataSet,
                     const std::string& fieldName,
                     const viskores::rendering::Canvas& canvas)
{
  VISKORES_TEST_ASSERT(dataSet.HasPointField(fieldName), "Point Field Not Found: " + fieldName);

  auto pointField = dataSet.GetPointField(fieldName);
  VISKORES_TEST_ASSERT(pointField.GetNumberOfValues() == canvas.GetWidth() * canvas.GetHeight(),
                       "wrong image dimensions");
  VISKORES_TEST_ASSERT(
    pointField.GetData().template IsType<viskores::cont::ArrayHandle<viskores::Vec4f_32>>(),
    "wrong ArrayHandle type");
  auto pixelPortal = pointField.GetData()
                       .template AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec4f_32>>()
                       .ReadPortal();

  auto colorPortal = canvas.GetColorBuffer().ReadPortal();

  VISKORES_TEST_ASSERT(test_equal_portals(pixelPortal, colorPortal));
}

void TestCreateImageDataSet(const viskores::rendering::Canvas& canvas)
{
  std::cout << "TestCreateImageDataSet" << std::endl;
  auto dataSet = canvas.GetDataSet("pixel-color");
  TestFilledImage(dataSet, "pixel-color", canvas);
}

void TestReadAndWritePNG(const viskores::rendering::Canvas& canvas,
                         std::string filename,
                         viskores::io::ImageWriterBase::PixelDepth pixelDepth)
{
  std::cout << "TestReadAndWritePNG - " << filename << std::endl;
  bool throws = false;
  try
  {
    viskores::io::ImageWriterPNG writer(filename);
    viskores::cont::DataSet dataSet;
    writer.WriteDataSet(dataSet);
  }
  catch (const viskores::cont::Error&)
  {
    throws = true;
  }
  VISKORES_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  {
    viskores::io::ImageWriterPNG writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNG reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
  }
  {
    viskores::io::ImageWriterPNG writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNG reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
    TestFilledImage(dataSet, reader.GetPointFieldName(), canvas);
  }
}

void TestReadAndWritePNM(const viskores::rendering::Canvas& canvas,
                         std::string filename,
                         viskores::io::ImageWriterBase::PixelDepth pixelDepth)
{
  std::cout << "TestReadAndWritePNM - " << filename << std::endl;
  bool throws = false;
  try
  {
    viskores::io::ImageWriterPNM writer(filename);
    viskores::cont::DataSet dataSet;
    writer.WriteDataSet(dataSet);
  }
  catch (const viskores::cont::Error&)
  {
    throws = true;
  }
  VISKORES_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  {
    viskores::io::ImageWriterPNM writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNM reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
  }
  {
    viskores::io::ImageWriterPNM writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet());
  }
  {
    viskores::io::ImageReaderPNM reader(filename);
    viskores::cont::DataSet dataSet = reader.ReadDataSet();
    TestFilledImage(dataSet, reader.GetPointFieldName(), canvas);
  }
}

void TestBaseImageMethods(const viskores::rendering::Canvas& canvas)
{
  TestCreateImageDataSet(canvas);
}

void TestWriteDataSetColorArrayTypes()
{
  std::cout << "TestWriteDataSetColorArrayTypes" << std::endl;

  {
    std::cout << "  UInt8 RGBA" << std::endl;
    auto colorArray = viskores::cont::make_ArrayHandle({
      viskores::Vec4ui_8(0, 64, 128, 255),
      viskores::Vec4ui_8(255, 128, 64, 0),
      viskores::Vec4ui_8(32, 96, 160, 224),
      viskores::Vec4ui_8(8, 16, 24, 32),
    });
    viskores::io::ImageWriterBase::ColorArrayType expectedPixels =
      viskores::cont::make_ArrayHandle({
        ExpectedRGB16(0.0f, 64.0f / 255.0f, 128.0f / 255.0f),
        ExpectedRGB16(1.0f, 128.0f / 255.0f, 64.0f / 255.0f),
        ExpectedRGB16(32.0f / 255.0f, 96.0f / 255.0f, 160.0f / 255.0f),
        ExpectedRGB16(8.0f / 255.0f, 16.0f / 255.0f, 24.0f / 255.0f),
      });

    TestWriteAndReadColorArray<viskores::io::ImageWriterPNG, viskores::io::ImageReaderPNG>(
      "pngVec4ui8Test.png", colorArray, expectedPixels);
    TestWriteAndReadColorArray<viskores::io::ImageWriterPNM, viskores::io::ImageReaderPNM>(
      "pnmVec4ui8Test.pnm", colorArray, expectedPixels);
  }

  {
    std::cout << "  Float32 RGB" << std::endl;
    auto colorArray = viskores::cont::make_ArrayHandle({
      viskores::Vec3f_32(0.0f, 0.25f, 0.5f),
      viskores::Vec3f_32(0.75f, 1.0f, 0.125f),
      viskores::Vec3f_32(0.875f, 0.625f, 0.375f),
      viskores::Vec3f_32(0.1f, 0.2f, 0.3f),
    });
    viskores::io::ImageWriterBase::ColorArrayType expectedPixels =
      viskores::cont::make_ArrayHandle({
        ExpectedRGB16(0.0f, 0.25f, 0.5f),
        ExpectedRGB16(0.75f, 1.0f, 0.125f),
        ExpectedRGB16(0.875f, 0.625f, 0.375f),
        ExpectedRGB16(0.1f, 0.2f, 0.3f),
      });

    TestWriteAndReadColorArray<viskores::io::ImageWriterPNG, viskores::io::ImageReaderPNG>(
      "pngVec3f32Test.png", colorArray, expectedPixels);
    TestWriteAndReadColorArray<viskores::io::ImageWriterPNM, viskores::io::ImageReaderPNM>(
      "pnmVec3f32Test.pnm", colorArray, expectedPixels);
  }

  {
    std::cout << "  Float64 RGBA" << std::endl;
    auto colorArray = viskores::cont::make_ArrayHandle({
      viskores::Vec4f_64(1.0, 0.0, 0.25, 0.5),
      viskores::Vec4f_64(0.125, 0.375, 0.625, 0.875),
      viskores::Vec4f_64(0.2, 0.4, 0.6, 0.8),
      viskores::Vec4f_64(0.33, 0.44, 0.55, 0.66),
    });
    viskores::io::ImageWriterBase::ColorArrayType expectedPixels =
      viskores::cont::make_ArrayHandle({
        ExpectedRGB16(1.0f, 0.0f, 0.25f),
        ExpectedRGB16(0.125f, 0.375f, 0.625f),
        ExpectedRGB16(0.2f, 0.4f, 0.6f),
        ExpectedRGB16(0.33f, 0.44f, 0.55f),
      });

    TestWriteAndReadColorArray<viskores::io::ImageWriterPNG, viskores::io::ImageReaderPNG>(
      "pngVec4f64Test.png", colorArray, expectedPixels);
    TestWriteAndReadColorArray<viskores::io::ImageWriterPNM, viskores::io::ImageReaderPNM>(
      "pnmVec4f64Test.pnm", colorArray, expectedPixels);
  }
}

void TestWriteDataSetBadColorArrayType()
{
  std::cout << "TestWriteDataSetBadColorArrayType" << std::endl;

  viskores::cont::DataSet dataSet =
    viskores::cont::DataSetBuilderUniform::Create(viskores::Id2(ImageWidth, ImageHeight));
  dataSet.AddPointField(ColorFieldName,
                        viskores::cont::make_ArrayHandle({
                          viskores::Vec4i_32(0, 64, 128, 255),
                          viskores::Vec4i_32(255, 128, 64, 0),
                          viskores::Vec4i_32(32, 96, 160, 224),
                          viskores::Vec4i_32(8, 16, 24, 32),
                        }));

  bool throws = false;
  try
  {
    viskores::io::ImageWriterPNG writer("pngBadColorArrayType.png");
    writer.WriteDataSet(dataSet, ColorFieldName);
  }
  catch (const viskores::cont::ErrorBadType&)
  {
    throws = true;
  }
  VISKORES_TEST_ASSERT(throws, "Image writer did not reject signed integer color channels");
}

void TestPNMImage(const viskores::rendering::Canvas& canvas)
{
  TestReadAndWritePNM(
    canvas, "pnmRGB8Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_8);
  TestReadAndWritePNM(
    canvas, "pnmRGB16Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_16);
}

void TestPNGImage(const viskores::rendering::Canvas& canvas)
{
  TestReadAndWritePNG(
    canvas, "pngRGB8Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_8);
  TestReadAndWritePNG(
    canvas, "pngRGB16Test.png", viskores::io::ImageWriterBase::PixelDepth::PIXEL_16);
}

void TestImage()
{
  viskores::rendering::Canvas canvas(16, 16);
  canvas.SetBackgroundColor(viskores::rendering::Color::red);
  canvas.Clear();
  // Line from top left to bottom right, ensures correct transposedness
  canvas.AddLine(-0.9, 0.9, 0.9, -0.9, 2.0f, viskores::rendering::Color::black);
  viskores::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  canvas.AddColorBar(colorBarBounds, viskores::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  canvas.SaveAs("baseline.ppm");

  TestBaseImageMethods(canvas);
  TestWriteDataSetColorArrayTypes();
  TestWriteDataSetBadColorArrayType();
  TestPNMImage(canvas);
  TestPNGImage(canvas);
}
}

int UnitTestImageWriter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestImage, argc, argv);
}
