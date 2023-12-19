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
#include <vtkm/io/VTKDataSetWriter.h>

void SaveDataAsVTKFile(vtkm::cont::DataSet data)
{
  vtkm::io::VTKDataSetWriter writer("data.vtk");

  writer.WriteDataSet(data);
}
////
//// END-EXAMPLE VTKDataSetWriter
////

////
//// BEGIN-EXAMPLE VTKDataSetReader
////
#include <vtkm/io/VTKDataSetReader.h>

vtkm::cont::DataSet OpenDataFromVTKFile()
{
  vtkm::io::VTKDataSetReader reader("data.vtk");

  return reader.ReadDataSet();
}
////
//// END-EXAMPLE VTKDataSetReader
////

////
//// BEGIN-EXAMPLE ImageReaderPNG
////
#include <vtkm/io/ImageReaderPNG.h>

vtkm::cont::DataSet OpenDataFromPNG()
{
  vtkm::io::ImageReaderPNG imageReader("data.png");
  imageReader.SetPointFieldName("pixel_colors");
  return imageReader.ReadDataSet();
}
////
//// END-EXAMPLE ImageReaderPNG
////

////
//// BEGIN-EXAMPLE ImageReaderPNM
////
#include <vtkm/io/ImageReaderPNM.h>

vtkm::cont::DataSet OpenDataFromPNM()
{
  vtkm::io::ImageReaderPNM imageReader("data.ppm");
  imageReader.SetPointFieldName("pixels");
  return imageReader.ReadDataSet();
}
////
//// END-EXAMPLE ImageReaderPNM
////

////
//// BEGIN-EXAMPLE ImageWriterPNG
////
#include <vtkm/io/ImageWriterPNG.h>

void WriteToPNG(const vtkm::cont::DataSet& dataSet)
{
  vtkm::io::ImageWriterPNG imageWriter("data.png");
  imageWriter.SetPixelDepth(vtkm::io::ImageWriterPNG::PixelDepth::PIXEL_16);
  imageWriter.WriteDataSet(dataSet);
}
////
//// END-EXAMPLE ImageWriterPNG
////

////
//// BEGIN-EXAMPLE ImageWriterPNM
////
#include <vtkm/io/ImageWriterPNM.h>

void WriteToPNM(const vtkm::cont::DataSet& dataSet)
{
  vtkm::io::ImageWriterPNM imageWriter("data.ppm");
  imageWriter.SetPixelDepth(vtkm::io::ImageWriterPNM::PixelDepth::PIXEL_16);
  imageWriter.WriteDataSet(dataSet);
}
////
//// END-EXAMPLE ImageWriterPNM
////

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>

namespace
{

void TestIO()
{
  std::cout << "Writing data" << std::endl;
  vtkm::cont::testing::MakeTestDataSet makeDataSet;
  vtkm::cont::DataSet createdData = makeDataSet.Make3DExplicitDataSetCowNose();
  SaveDataAsVTKFile(createdData);

  std::cout << "Reading data" << std::endl;
  vtkm::cont::DataSet readData = OpenDataFromVTKFile();

  const vtkm::cont::CellSet* createdCellSet = createdData.GetCellSet().GetCellSetBase();
  const vtkm::cont::CellSet* readCellSet = readData.GetCellSet().GetCellSetBase();
  VTKM_TEST_ASSERT(createdCellSet->GetNumberOfCells() == readCellSet->GetNumberOfCells(),
                   "Createded and read data do not match.");
  VTKM_TEST_ASSERT(createdCellSet->GetNumberOfPoints() ==
                     readCellSet->GetNumberOfPoints(),
                   "Createded and read data do not match.");

  std::cout << "Reading and writing image data" << std::endl;
  vtkm::Bounds colorBarBounds(-0.8, -0.6, -0.8, 0.8, 0, 0);
  vtkm::rendering::Canvas canvas(64, 64);
  canvas.SetBackgroundColor(vtkm::rendering::Color::blue);
  canvas.Clear();
  canvas.AddColorBar(colorBarBounds, vtkm::cont::ColorTable("inferno"), false);
  canvas.BlendBackground();
  vtkm::cont::DataSet imageSource = canvas.GetDataSet("color", nullptr);

  WriteToPNG(imageSource);
  WriteToPNM(imageSource);

  using CheckType = typename vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;

  readData = OpenDataFromPNG();
  VTKM_TEST_ASSERT(readData.HasPointField("pixel_colors"),
                   "Point Field Not Found: pixel-data");
  vtkm::cont::Field colorField = readData.GetPointField("pixel_colors");
  VTKM_TEST_ASSERT(colorField.GetNumberOfValues() == 64 * 64, "wrong image dimensions");
  VTKM_TEST_ASSERT(colorField.GetData().IsType<CheckType>(), "wrong ArrayHandle type");

  readData = OpenDataFromPNM();
  VTKM_TEST_ASSERT(readData.HasPointField("pixels"),
                   "Point Field Not Found: pixel-data");
  colorField = readData.GetPointField("pixels");
  VTKM_TEST_ASSERT(colorField.GetNumberOfValues() == 64 * 64, "wrong image dimensions");
  VTKM_TEST_ASSERT(colorField.GetData().IsType<CheckType>(), "wrong ArrayHandle type");
}

} // namespace

int GuideExampleIO(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestIO, argc, argv);
}
