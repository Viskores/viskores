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

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorExecution.h>
#include <viskores/cont/StorageList.h>
#include <viskores/filter/field_transform/RGBToLab.h>
#include <viskores/filter/image_processing/ImageDifference.h>
#include <viskores/filter/image_processing/SSIM.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{

// Computes the average value of the ssimValues.
viskores::Float32 ComputeMinkowski(const viskores::cont::UnknownArrayHandle& ssimValues)
{
  viskores::Float32 minkowski{ 0 };

  auto doReduce = [&](auto ssimArray)
  {
    using ValueType = typename decltype(ssimArray)::ValueType;
    ValueType sum = viskores::cont::Algorithm::Reduce(ssimArray, ValueType{ 0 });
    ValueType average = sum / ssimArray.GetNumberOfValues();

    // The range of ssim values is [-1, 1]. By doing 1 - value, we change the range to [0, 2]
    minkowski = 1.0f - static_cast<viskores::Float32>(average);
  };
  ssimValues.CastAndCallForTypes<viskores::TypeListFieldScalar, viskores::cont::StorageListBasic>(
    doReduce);

  return minkowski;
}

viskores::Float32 ErrorMetric(const viskores::cont::UnknownArrayHandle& ssimValues)
{
  viskores::Float32 minkowski = ComputeMinkowski(ssimValues);

  // Taking the square root penalizes misses harder.
  return viskores::Sqrt(minkowski);
}

} // anonymous namespace

TestEqualResult test_equal_images(viskores::rendering::View& view,
                                  const std::vector<std::string>& fileNames,
                                  const viskores::IdComponent& averageRadius,
                                  const viskores::IdComponent& pixelShiftRadius,
                                  const viskores::FloatDefault& allowedPixelErrorRatio,
                                  const viskores::FloatDefault& threshold,
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

TestEqualResult test_equal_images(const std::vector<std::string>& fileNames,
                                  viskores::rendering::View& view,
                                  const SSIMCompareOptions& options,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  view.Paint();
  return test_equal_images(fileNames, view.GetCanvas(), options, writeDiff, returnOnPass);
}

TestEqualResult test_equal_images(const viskores::rendering::Canvas& canvas,
                                  const std::vector<std::string>& fileNames,
                                  const viskores::IdComponent& averageRadius,
                                  const viskores::IdComponent& pixelShiftRadius,
                                  const viskores::FloatDefault& allowedPixelErrorRatio,
                                  const viskores::FloatDefault& threshold,
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

TestEqualResult test_equal_images(const std::vector<std::string>& fileNames,
                                  const viskores::rendering::Canvas& canvas,
                                  const SSIMCompareOptions& options,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  canvas.RefreshColorBuffer();
  return test_equal_images(fileNames,
                           canvas.GetColorBuffer(),
                           { canvas.GetWidth(), canvas.GetHeight() },
                           options,
                           writeDiff,
                           returnOnPass);
}

TestEqualResult test_equal_images(const viskores::cont::DataSet& dataset,
                                  const std::vector<std::string>& fileNames,
                                  const viskores::IdComponent& averageRadius,
                                  const viskores::IdComponent& pixelShiftRadius,
                                  const viskores::FloatDefault& allowedPixelErrorRatio,
                                  const viskores::FloatDefault& threshold,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  viskores::cont::ScopedRuntimeDeviceTracker runtime(viskores::cont::DeviceAdapterTagAny{});
  TestEqualResult testResults;

  if (fileNames.empty())
  {
    testResults.PushMessage("No valid image file names were provided");
    return testResults;
  }

  const std::string testImageName = viskores::cont::testing::Testing::WriteDirPath(
    viskores::io::PrefixStringToFilename(fileNames[0], "test-"));
  viskores::io::WriteImageFile(dataset, testImageName, dataset.GetField(0).GetName());

  std::stringstream dartXML;

  dartXML << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
  dartXML << testImageName;
  dartXML << "</DartMeasurementFile>\n";

  for (const auto& fileName : fileNames)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "testing image file: " << fileName);
    TestEqualResult imageResult;
    viskores::cont::DataSet imageDataSet;
    const std::string testImagePath =
      viskores::cont::testing::Testing::RegressionImagePath(fileName);

    try
    {
      imageDataSet = viskores::io::ReadImageFile(testImagePath, "baseline-image");
    }
    catch (const viskores::cont::ErrorExecution& error)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error, error.GetMessage());
      imageResult.PushMessage(error.GetMessage());

      const std::string outputImagePath = viskores::cont::testing::Testing::WriteDirPath(fileName);
      viskores::io::WriteImageFile(dataset, outputImagePath, dataset.GetField(0).GetName());

      imageResult.PushMessage("File '" + fileName +
                              "' did not exist but has been generated here: " + outputImagePath);
      testResults.PushMessage(imageResult.GetMergedMessage());
      continue;
    }
    catch (const viskores::cont::ErrorBadValue& error)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error, error.GetMessage());
      imageResult.PushMessage(error.GetMessage());
      imageResult.PushMessage("Unsupported file type for image: " + fileName);
      testResults.PushMessage(imageResult.GetMergedMessage());
      continue;
    }

    dartXML << "<DartMeasurementFile name=\"BaselineImage\" type=\"image/png\">";
    dartXML << testImagePath;
    dartXML << "</DartMeasurementFile>\n";

    imageDataSet.AddPointField("generated-image", dataset.GetField(0).GetData());
    viskores::filter::image_processing::ImageDifference filter;
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
      const std::string diffName = viskores::cont::testing::Testing::WriteDirPath(
        viskores::io::PrefixStringToFilename(fileName, "diff-"));
      viskores::io::WriteImageFile(resultDataSet, diffName, "image-diff");
      dartXML << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
      dartXML << diffName;
      dartXML << "</DartMeasurementFile>\n";
    }

    if (imageResult && returnOnPass)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Test passed for image " << fileName);
      if (!testResults)
      {
        VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                       "Other image errors: " << testResults.GetMergedMessage());
      }
      return imageResult;
    }

    testResults.PushMessage(imageResult.GetMergedMessage());
  }

  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 "Test Results: " << testResults.GetMergedMessage());

  if (!testResults)
  {
    std::cout << dartXML.str();
  }

  return testResults;
}

TestEqualResult test_equal_images(const std::vector<std::string>& fileNames,
                                  const viskores::cont::UnknownArrayHandle& colors,
                                  const viskores::Id2& dimensions,
                                  const SSIMCompareOptions& options,
                                  const bool& writeDiff,
                                  const bool& returnOnPass)
{
  viskores::cont::ScopedRuntimeDeviceTracker runtime(viskores::cont::DeviceAdapterTagAny{});
  TestEqualResult testResults;

  if (fileNames.empty())
  {
    testResults.PushMessage("No valid image file names were provided");
    return testResults;
  }

  VISKORES_ASSERT(colors.GetNumberOfValues() == (dimensions[0] * dimensions[1]));

  viskores::cont::UnknownArrayHandle generatedColors;

  // If the image has an alpha parameter, strip it.
  if (colors.GetNumberOfComponentsFlat() > 3)
  {
    auto removeAlpha = [&](auto recombineVecArray)
    {
      using ComponentType =
        std::decay_t<typename decltype(recombineVecArray)::ValueType::ComponentType>;
      // Construct an array with the first three color channels.
      viskores::cont::ArrayHandleRecombineVec<ComponentType> noAlphaColors;
      noAlphaColors.AppendComponentArray(recombineVecArray.GetComponentArray(0));
      noAlphaColors.AppendComponentArray(recombineVecArray.GetComponentArray(1));
      noAlphaColors.AppendComponentArray(recombineVecArray.GetComponentArray(2));

      generatedColors = viskores::cont::ArrayHandle<viskores::Vec<ComponentType, 3>>{};

      viskores::cont::ArrayCopy(noAlphaColors, generatedColors);
    };
    colors.CastAndCallWithExtractedArray(removeAlpha);
  }
  else
  {
    generatedColors = colors;
  }

  viskores::cont::DataSetBuilderUniform builder;
  viskores::cont::DataSet imageData = builder.Create(dimensions);
  imageData.AddPointField("colors", generatedColors);

  const std::string testImageName = viskores::cont::testing::Testing::WriteDirPath(
    viskores::io::PrefixStringToFilename(fileNames[0], "test-"));
  viskores::io::WriteImageFile(imageData, testImageName, "colors");

  std::stringstream dartXML;

  dartXML << "<DartMeasurementFile name=\"TestImage\" type=\"image/png\">";
  dartXML << testImageName;
  dartXML << "</DartMeasurementFile>\n";

  for (const auto& fileName : fileNames)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "testing image file: " << fileName);
    TestEqualResult imageResult;
    viskores::cont::DataSet imageDataSet;
    const std::string testImagePath =
      viskores::cont::testing::Testing::RegressionImagePath(fileName);

    try
    {
      imageDataSet = viskores::io::ReadImageFile(testImagePath, "baseline-image");
    }
    catch (const viskores::cont::ErrorExecution& error)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error, error.GetMessage());
      imageResult.PushMessage(error.GetMessage());

      const std::string outputImagePath = viskores::cont::testing::Testing::WriteDirPath(fileName);
      viskores::io::WriteImageFile(imageData, outputImagePath, "colors");

      imageResult.PushMessage("File '" + fileName +
                              "' did not exist but has been generated here: " + outputImagePath);
      testResults.PushMessage(imageResult.GetMergedMessage());
      continue;
    }
    catch (const viskores::cont::ErrorBadValue& error)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Error, error.GetMessage());
      imageResult.PushMessage(error.GetMessage());
      imageResult.PushMessage("Unsupported file type for image: " + fileName);
      testResults.PushMessage(imageResult.GetMergedMessage());
      continue;
    }

    dartXML << "<DartMeasurementFile name=\"BaselineImage\" type=\"image/png\">";
    dartXML << testImagePath;
    dartXML << "</DartMeasurementFile>\n";

    imageDataSet.AddPointField("generated-image", generatedColors);

    viskores::filter::field_transform::RGBToLab rgb2lab;

    rgb2lab.SetActiveField("baseline-image");
    rgb2lab.SetOutputFieldName("baseline-image-lab");
    imageDataSet = rgb2lab.Execute(imageDataSet);

    rgb2lab.SetActiveField("generated-image");
    rgb2lab.SetOutputFieldName("generated-image-lab");
    imageDataSet = rgb2lab.Execute(imageDataSet);

    viskores::filter::image_processing::SSIM filter;
    filter.SetPrimaryField("baseline-image-lab");
    filter.SetSecondaryField("generated-image-lab");
    filter.SetOutputFieldName("ssim");
    filter.SetDynamicRange(100.0);
    filter.SetPatchRadius(options.PatchRadius);
    viskores::cont::DataSet ssimDataSet = filter.Execute(imageDataSet);

    viskores::Float32 error = ErrorMetric(ssimDataSet.GetField("ssim").GetData());
    VISKORES_LOG_S(viskores::cont::LogLevel::Info, "SSIM error metric: " << error);

    if (error > options.Tolerance)
    {
      imageResult.PushMessage("Accumulative SSIM was not within the expected threshold for: " +
                              fileName);
    }

    if (writeDiff)
    {
      const std::string diffName = viskores::cont::testing::Testing::WriteDirPath(
        viskores::io::PrefixStringToFilename(fileName, "diff-"));
      viskores::io::WriteImageFile(ssimDataSet, diffName, "ssim");
      dartXML << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/png\">";
      dartXML << diffName;
      dartXML << "</DartMeasurementFile>\n";
    }

    if (imageResult && returnOnPass)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Test passed for image " << fileName);
      if (!testResults)
      {
        VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                       "Other image errors: " << testResults.GetMergedMessage());
      }
      return imageResult;
    }

    testResults.PushMessage(imageResult.GetMergedMessage());
  }

  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 "Test Results: " << testResults.GetMergedMessage());

  if (!testResults)
  {
    std::cout << dartXML.str();
  }

  return testResults;
}
