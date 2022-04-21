//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/rendering/testing/Testing.h>

TestEqualResult test_equal_images(vtkm::rendering::View& view,
                                  const std::vector<std::string>& fileNames,
                                  const vtkm::IdComponent& averageRadius,
                                  const vtkm::IdComponent& pixelShiftRadius,
                                  const vtkm::FloatDefault& allowedPixelErrorRatio,
                                  const vtkm::FloatDefault& threshold,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  view.Paint();
  return test_equal_images(view.GetCanvas(),
                           fileNames,
                           averageRadius,
                           pixelShiftRadius,
                           allowedPixelErrorRatio,
                           threshold,
                           writeDiff,
                           returnOnPass);
}

TestEqualResult test_equal_images(const vtkm::rendering::Canvas& canvas,
                                  const std::vector<std::string>& fileNames,
                                  const vtkm::IdComponent& averageRadius,
                                  const vtkm::IdComponent& pixelShiftRadius,
                                  const vtkm::FloatDefault& allowedPixelErrorRatio,
                                  const vtkm::FloatDefault& threshold,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  canvas.RefreshColorBuffer();
  return test_equal_images(canvas.GetDataSet(),
                           fileNames,
                           averageRadius,
                           pixelShiftRadius,
                           allowedPixelErrorRatio,
                           threshold,
                           writeDiff,
                           returnOnPass);
}

TestEqualResult test_equal_images(const vtkm::cont::DataSet& dataset,
                                  const std::vector<std::string>& fileNames,
                                  const vtkm::IdComponent& averageRadius,
                                  const vtkm::IdComponent& pixelShiftRadius,
                                  const vtkm::FloatDefault& allowedPixelErrorRatio,
                                  const vtkm::FloatDefault& threshold,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  vtkm::cont::ScopedRuntimeDeviceTracker runtime(vtkm::cont::DeviceAdapterTagAny{});
  TestEqualResult testResults;

  if (fileNames.empty())
  {
    testResults.PushMessage("No valid image file names were provided");
    return testResults;
  }

  const std::string testImageName = vtkm::cont::testing::Testing::WriteDirPath(
    vtkm::io::PrefixStringToFilename(fileNames[0], "test-"));
  vtkm::io::WriteImageFile(dataset, testImageName, dataset.GetField(0).GetName());

  std::stringstream dartXML;

  dartXML << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
  dartXML << testImageName;
  dartXML << "</DartMeasurementFile>\n";

  for (const auto& fileName : fileNames)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "testing image file: " << fileName);
    TestEqualResult imageResult;
    vtkm::cont::DataSet imageDataSet;
    const std::string testImagePath = vtkm::cont::testing::Testing::RegressionImagePath(fileName);

    try
    {
      imageDataSet = vtkm::io::ReadImageFile(testImagePath, "baseline-image");
    }
    catch (const vtkm::cont::ErrorExecution& error)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error, error.GetMessage());
      imageResult.PushMessage(error.GetMessage());

      const std::string outputImagePath = vtkm::cont::testing::Testing::WriteDirPath(fileName);
      vtkm::io::WriteImageFile(dataset, outputImagePath, dataset.GetField(0).GetName());

      imageResult.PushMessage("File '" + fileName +
                              "' did not exist but has been generated here: " + outputImagePath);
      testResults.PushMessage(imageResult.GetMergedMessage());
      continue;
    }
    catch (const vtkm::cont::ErrorBadValue& error)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error, error.GetMessage());
      imageResult.PushMessage(error.GetMessage());
      imageResult.PushMessage("Unsupported file type for image: " + fileName);
      testResults.PushMessage(imageResult.GetMergedMessage());
      continue;
    }

    dartXML << "<DartMeasurementFile name=\"BaselineImage\" type=\"image/png\">";
    dartXML << testImagePath;
    dartXML << "</DartMeasurementFile>\n";

    imageDataSet.AddPointField("generated-image", dataset.GetField(0).GetData());
    vtkm::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("baseline-image");
    filter.SetSecondaryField("generated-image");
    filter.SetAverageRadius(averageRadius);
    filter.SetPixelShiftRadius(pixelShiftRadius);
    filter.SetAllowedPixelErrorRatio(allowedPixelErrorRatio);
    filter.SetPixelDiffThreshold(threshold);
    auto resultDataSet = filter.Execute(imageDataSet);

    if (!filter.GetImageDiffWithinThreshold())
    {
      imageResult.PushMessage("Image Difference was not within the expected threshold for: " +
                              fileName);
    }

    if (writeDiff && resultDataSet.HasPointField("image-diff"))
    {
      const std::string diffName = vtkm::cont::testing::Testing::WriteDirPath(
        vtkm::io::PrefixStringToFilename(fileName, "diff-"));
      vtkm::io::WriteImageFile(resultDataSet, diffName, "image-diff");
      dartXML << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
      dartXML << diffName;
      dartXML << "</DartMeasurementFile>\n";
    }

    if (imageResult && returnOnPass)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Test passed for image " << fileName);
      if (!testResults)
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Other image errors: " << testResults.GetMergedMessage());
      }
      return imageResult;
    }

    testResults.PushMessage(imageResult.GetMergedMessage());
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Test Results: " << testResults.GetMergedMessage());

  if (!testResults)
  {
    std::cout << dartXML.str();
  }

  return testResults;
}
