//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/image_processing/ImageDifference.h>

#include <random>
#include <vector>

namespace
{

template <typename T>
void CreateData(std::size_t numPts,
                const T& fudgeFactor,
                std::vector<vtkm::Vec<T, 4>>& primary,
                std::vector<vtkm::Vec<T, 4>>& secondary)
{
  primary.resize(numPts);
  secondary.resize(numPts);
  for (std::size_t i = 0; i < numPts; ++i)
  {
    primary[i] = vtkm::Vec<T, 4>(1, 1, 1, 1);
    secondary[i] = vtkm::Vec<T, 4>(fudgeFactor, 1, 1, 1);
  }
}


void CheckResult(const std::vector<vtkm::Vec4f>& expectedDiff,
                 const std::vector<vtkm::FloatDefault>& expectedThreshold,
                 const vtkm::cont::DataSet& output,
                 bool inThreshold,
                 bool expectedInThreshold)
{
  VTKM_TEST_ASSERT(output.HasPointField("image-diff"), "Output field is missing.");

  vtkm::cont::ArrayHandle<vtkm::Vec4f> outputArray;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> thresholdArray;
  output.GetPointField("image-diff").GetData().AsArrayHandle(outputArray);
  output.GetPointField("threshold-output").GetData().AsArrayHandle(thresholdArray);

  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == static_cast<vtkm::Id>(expectedDiff.size()),
                   "Field sizes wrong");

  auto outPortal = outputArray.ReadPortal();

  for (vtkm::Id j = 0; j < outputArray.GetNumberOfValues(); j++)
  {
    vtkm::Vec4f val1 = outPortal.Get(j);
    vtkm::Vec4f val2 = expectedDiff[j];
    VTKM_TEST_ASSERT(test_equal(val1, val2), "Wrong result for image-diff");
  }

  auto thresholdPortal = thresholdArray.ReadPortal();
  for (vtkm::Id j = 0; j < thresholdArray.GetNumberOfValues(); j++)
  {
    vtkm::Vec4f val1 = thresholdPortal.Get(j);
    vtkm::Vec4f val2 = expectedThreshold[j];
    VTKM_TEST_ASSERT(test_equal(val1, val2), "Wrong result for threshold output");
  }

  if (expectedInThreshold)
  {
    VTKM_TEST_ASSERT(inThreshold, "Diff image was not within the error threshold");
  }
  else
  {
    VTKM_TEST_ASSERT(!inThreshold, "Diff image was found to be within the error threshold");
  }
}

vtkm::cont::DataSet FillDataSet(const vtkm::FloatDefault& fudgeFactor)
{
  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet dataSet = builder.Create(vtkm::Id2(5, 5));

  std::vector<vtkm::Vec4f> primary;
  std::vector<vtkm::Vec4f> secondary;
  CreateData(static_cast<std::size_t>(25), fudgeFactor, primary, secondary);
  dataSet.AddPointField("primary", primary);
  dataSet.AddPointField("secondary", secondary);

  return dataSet;
}

void TestImageDifference()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Testing ImageDifference Filter");

  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Matching Images");
    auto dataSet = FillDataSet(static_cast<vtkm::FloatDefault>(1));
    vtkm::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(0);
    vtkm::cont::DataSet result = filter.Execute(dataSet);

    std::vector<vtkm::Vec4f> expectedDiff = {
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
    };
    std::vector<vtkm::FloatDefault> expectedThreshold = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }

  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Matching Images with Average");
    auto dataSet = FillDataSet(static_cast<vtkm::FloatDefault>(1));
    vtkm::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(1);
    filter.SetAverageRadius(1);
    vtkm::cont::DataSet result = filter.Execute(dataSet);

    std::vector<vtkm::Vec4f> expectedDiff = {
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 },
      { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }
    };
    std::vector<vtkm::FloatDefault> expectedThreshold = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }

  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Non Matching Images (Different R pixel)");
    auto dataSet = FillDataSet(static_cast<vtkm::FloatDefault>(3));
    vtkm::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(0);
    vtkm::cont::DataSet result = filter.Execute(dataSet);

    std::vector<vtkm::Vec4f> expectedDiff = {
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }
    };
    std::vector<vtkm::FloatDefault> expectedThreshold = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                          2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), false);
  }

  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Non Matching Images (Different R pixel)");
    auto dataSet = FillDataSet(static_cast<vtkm::FloatDefault>(3));
    vtkm::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(0.05f);
    filter.SetPixelShiftRadius(0);
    filter.SetAllowedPixelErrorRatio(1.00f);
    vtkm::cont::DataSet result = filter.Execute(dataSet);

    std::vector<vtkm::Vec4f> expectedDiff = {
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }
    };
    std::vector<vtkm::FloatDefault> expectedThreshold = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                          2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }

  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Non Matching Images (Different R pixel), Large Threshold");
    auto dataSet = FillDataSet(static_cast<vtkm::FloatDefault>(3));
    vtkm::filter::image_processing::ImageDifference filter;
    filter.SetPrimaryField("primary");
    filter.SetSecondaryField("secondary");
    filter.SetPixelDiffThreshold(3.0f);
    filter.SetPixelShiftRadius(0);
    vtkm::cont::DataSet result = filter.Execute(dataSet);

    std::vector<vtkm::Vec4f> expectedDiff = {
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 },
      { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }, { 2, 0, 0, 0 }
    };
    std::vector<vtkm::FloatDefault> expectedThreshold = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                                          2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
    CheckResult(
      expectedDiff, expectedThreshold, result, filter.GetImageDiffWithinThreshold(), true);
  }
}
} // anonymous namespace

int UnitTestImageDifferenceFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestImageDifference, argc, argv);
}
