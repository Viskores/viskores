//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

////
//// BEGIN-EXAMPLE VTKDataSetWriter
////
#include <viskores/io/VTKDataSetWriter.h>

void SaveDataAsVTKFile(viskores::cont::DataSet data)
{
  viskores::io::VTKDataSetWriter writer("data.vtk");

  writer.WriteDataSet(data);
}
////
//// END-EXAMPLE VTKDataSetWriter
////

////
//// BEGIN-EXAMPLE VTKDataSetReader
////
#include <viskores/io/VTKDataSetReader.h>

viskores::cont::DataSet OpenDataFromVTKFile()
{
  viskores::io::VTKDataSetReader reader("data.vtk");

  return reader.ReadDataSet();
}
////
//// END-EXAMPLE VTKDataSetReader
////

////
//// BEGIN-EXAMPLE ImageReaderPNG
////
#include <viskores/io/ImageReaderPNG.h>

viskores::cont::DataSet OpenDataFromPNG()
{
  viskores::io::ImageReaderPNG imageReader("data.png");
  imageReader.SetPointFieldName("pixel_colors");
  return imageReader.ReadDataSet();
}
////
//// END-EXAMPLE ImageReaderPNG
////

////
//// BEGIN-EXAMPLE ImageReaderPNM
////
#include <viskores/io/ImageReaderPNM.h>

viskores::cont::DataSet OpenDataFromPNM()
{
  viskores::io::ImageReaderPNM imageReader("data.ppm");
  imageReader.SetPointFieldName("pixels");
  return imageReader.ReadDataSet();
}
////
//// END-EXAMPLE ImageReaderPNM
////

////
//// BEGIN-EXAMPLE ImageWriterPNG
////
#include <viskores/io/ImageWriterPNG.h>

void WriteToPNG(const viskores::cont::DataSet& dataSet)
{
  viskores::io::ImageWriterPNG imageWriter("data.png");
  imageWriter.SetPixelDepth(viskores::io::ImageWriterPNG::PixelDepth::PIXEL_16);
  imageWriter.WriteDataSet(dataSet);
}
////
//// END-EXAMPLE ImageWriterPNG
////

////
//// BEGIN-EXAMPLE ImageWriterPNM
////
#include <viskores/io/ImageWriterPNM.h>

void WriteToPNM(const viskores::cont::DataSet& dataSet)
{
  viskores::io::ImageWriterPNM imageWriter("data.ppm");
  imageWriter.SetPixelDepth(viskores::io::ImageWriterPNM::PixelDepth::PIXEL_16);
  imageWriter.WriteDataSet(dataSet);
}
////
//// END-EXAMPLE ImageWriterPNM
////

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Color.h>

namespace
{

void TestIO()
{
  std::cout << "Writing data" << std::endl;
  viskores::cont::testing::MakeTestDataSet makeDataSet;
  viskores::cont::DataSet createdData = makeDataSet.Make3DExplicitDataSetCowNose();
  SaveDataAsVTKFile(createdData);

  std::cout << "Reading data" << std::endl;
  viskores::cont::DataSet readData = OpenDataFromVTKFile();

  const viskores::cont::CellSet* createdCellSet = createdData.GetCellSet().GetCellSetBase();
  const viskores::cont::CellSet* readCellSet = readData.GetCellSet().GetCellSetBase();
  VISKORES_TEST_ASSERT(createdCellSet->GetNumberOfCells() == readCellSet->GetNumberOfCells(),
                   "Createded and read data do not match.");
  VISKORES_TEST_ASSERT(createdCellSet->GetNumberOfPoints() ==
                     readCellSet->GetNumberOfPoints(),
                   "Createded and read data do not match.");

  std::cout << "Reading and writing image data" << std::endl;
  viskores::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  viskores::rendering::Canvas canvas(64, 64);
  canvas.SetBackgroundColor(viskores::rendering::Color::blue);
  canvas.Clear();
  canvas.AddColorBar(colorBarBounds, viskores::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  viskores::cont::DataSet imageSource = canvas.GetDataSet("color", nullptr);

  WriteToPNG(imageSource);
  WriteToPNM(imageSource);

  using CheckType = typename viskores::cont::ArrayHandle<viskores::Vec4f_32>;

  readData = OpenDataFromPNG();
  VISKORES_TEST_ASSERT(readData.HasPointField("pixel_colors"),
                   "Point Field Not Found: pixel-data");
  viskores::cont::Field colorField = readData.GetPointField("pixel_colors");
  VISKORES_TEST_ASSERT(colorField.GetNumberOfValues() == 64 * 64, "wrong image dimensions");
  VISKORES_TEST_ASSERT(colorField.GetData().IsType<CheckType>(), "wrong ArrayHandle type");

  readData = OpenDataFromPNM();
  VISKORES_TEST_ASSERT(readData.HasPointField("pixels"),
                   "Point Field Not Found: pixel-data");
  colorField = readData.GetPointField("pixels");
  VISKORES_TEST_ASSERT(colorField.GetNumberOfValues() == 64 * 64, "wrong image dimensions");
  VISKORES_TEST_ASSERT(colorField.GetData().IsType<CheckType>(), "wrong ArrayHandle type");
}

} // namespace

int GuideExampleIO(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestIO, argc, argv);
}
