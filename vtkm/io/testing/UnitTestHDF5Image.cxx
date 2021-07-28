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
#include <vtkm/io/ImageReaderHDF5.h>
#include <vtkm/io/ImageWriterHDF5.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>

#include <string>
namespace
{

using namespace vtkm::io;
using namespace vtkm::rendering;

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
  vtkm::cont::ArrayHandle<vtkm::Vec4f_32> pixelArray;
  pointField.GetData().AsArrayHandle(pixelArray);
  auto pixelPortal = pixelArray.ReadPortal();

  auto colorPortal = canvas.GetColorBuffer().ReadPortal();

  VTKM_TEST_ASSERT(test_equal_portals(pixelPortal, colorPortal));
}

void TestCreateImageDataSet(const vtkm::rendering::Canvas& canvas)
{
  std::cout << "TestCreateImageDataSet" << std::endl;
  auto dataSet = canvas.GetDataSet("pixel-color");
  TestFilledImage(dataSet, "pixel-color", canvas);
}

void TestReadAndWriteHDF5(const vtkm::rendering::Canvas& canvas,
                          std::string filename,
                          vtkm::io::ImageWriterBase::PixelDepth pixelDepth)
{
  std::cout << "TestReadAndWriteHDF5 - " << filename << std::endl;
  bool throws = false;
  try
  {
    vtkm::io::ImageWriterHDF5 writer(filename);
    vtkm::cont::DataSet dataSet;
    writer.WriteDataSet(dataSet, "color");
  }
  catch (const vtkm::cont::Error&)
  {
    throws = true;
  }
  VTKM_TEST_ASSERT(throws, "Fill Image did not throw with empty data");

  {
    vtkm::io::ImageWriterHDF5 writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet(), "color");
  }
  {
    vtkm::io::ImageReaderHDF5 reader(filename);
    vtkm::cont::DataSet dataSet = reader.ReadDataSet();
  }
  {
    vtkm::io::ImageWriterHDF5 writer(filename);
    writer.SetPixelDepth(pixelDepth);
    writer.WriteDataSet(canvas.GetDataSet(), "color");
  }
  {
    vtkm::io::ImageReaderHDF5 reader(filename);
    vtkm::cont::DataSet dataSet = reader.ReadDataSet();
    TestFilledImage(dataSet, reader.GetPointFieldName(), canvas);
  }
}

void TestHDF5Image()
{
  vtkm::rendering::Canvas canvas(16, 16);
  canvas.SetBackgroundColor(vtkm::rendering::Color::red);
  canvas.Clear();
  // Line from top left to bottom right, ensures correct transposedness
  canvas.AddLine(-0.9, 0.9, 0.9, -0.9, 2.0f, vtkm::rendering::Color::black);
  vtkm::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  canvas.AddColorBar(colorBarBounds, vtkm::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();

  TestReadAndWriteHDF5(canvas, "hdf5RGB8Test.h5", vtkm::io::ImageWriterBase::PixelDepth::PIXEL_8);
  TestReadAndWriteHDF5(canvas, "hdf5RGB16Test.h5", vtkm::io::ImageWriterBase::PixelDepth::PIXEL_16);
}

} // namespace

int UnitTestHDF5Image(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestHDF5Image, argc, argv);
}
