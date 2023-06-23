//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/image_processing/ComputeMoments.h>
#include <vtkm/source/Wavelet.h>

#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/rendering/testing/Testing.h>

namespace
{

void TestComputeMoments()
{
  vtkm::source::Wavelet source;
  vtkm::cont::DataSet data = source.Execute();

  vtkm::filter::image_processing::ComputeMoments filter;
  filter.SetActiveField("RTData");
  filter.SetOrder(2);
  filter.SetRadius(2);
  vtkm::cont::DataSet result = filter.Execute(data);

  vtkm::rendering::testing::RenderTestOptions testOptions;
  testOptions.AllowedPixelErrorRatio = 0.001f;
  testOptions.ColorTable = vtkm::cont::ColorTable("inferno");
  testOptions.EnableAnnotations = false;
  vtkm::rendering::testing::RenderTest(result, "index", "filter/moments.png", testOptions);
  vtkm::rendering::testing::RenderTest(result, "index0", "filter/moments0.png", testOptions);
  vtkm::rendering::testing::RenderTest(result, "index12", "filter/moments12.png", testOptions);
}
} // namespace

int RenderTestComputeMoments(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestComputeMoments, argc, argv);
}
