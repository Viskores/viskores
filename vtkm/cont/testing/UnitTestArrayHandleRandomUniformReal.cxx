//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleRandomUniformReal.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DescriptiveStatistics.h>

void TestRangeBounds()
{
  // the random numbers should fall into the range of [0, 1).
  auto array = vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(1000000, { 0xceed });
  auto portal = array.ReadPortal();
  for (vtkm::Id i = 0; i < array.GetNumberOfValues(); ++i)
  {
    auto value = portal.Get(i);
    VTKM_TEST_ASSERT(0.0 <= value && value < 1.0);
  }
}

void TestStatisticsProperty()
{
  auto array = vtkm::cont::ArrayHandleRandomUniformReal<vtkm::Float32>(1000000, { 0xceed });
  auto result = vtkm::worklet::DescriptiveStatistics::Run(array);

  VTKM_TEST_ASSERT(test_equal(result.Mean(), 0.5, 0.001));
  VTKM_TEST_ASSERT(test_equal(result.SampleVariance(), 1.0 / 12.0, 0.001));
}

void TestArrayHandleUniformReal()
{
  TestRangeBounds();
  TestStatisticsProperty();
}

int UnitTestArrayHandleRandomUniformReal(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleUniformReal, argc, argv);
}
