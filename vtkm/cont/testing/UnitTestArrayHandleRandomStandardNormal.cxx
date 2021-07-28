//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleRandomStandardNormal.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DescriptiveStatistics.h>

void TestArrayHandleStandardNormal()
{
  auto array = vtkm::cont::ArrayHandleRandomStandardNormal<vtkm::Float32>(50000, { 0xceed });
  auto stats = vtkm::worklet::DescriptiveStatistics::Run(array);

  VTKM_TEST_ASSERT(test_equal(stats.Mean(), 0, 0.01));
  VTKM_TEST_ASSERT(test_equal(stats.PopulationStddev(), 1, 0.01));
  VTKM_TEST_ASSERT(test_equal(stats.Skewness(), 0.0f, 1.0f / 100));
  VTKM_TEST_ASSERT(test_equal(stats.Kurtosis(), 3.0f, 1.0f / 100));
}

int UnitTestArrayHandleRandomStandardNormal(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleStandardNormal, argc, argv);
}
