//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/Magnitude.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestMagnitude()
{
  std::cout << "Testing Magnitude Worklet" << std::endl;

  vtkm::worklet::Magnitude magnitudeWorklet;

  using ArrayReturnType = vtkm::cont::ArrayHandle<vtkm::Float64>;
  using ArrayVectorType = vtkm::cont::ArrayHandle<vtkm::Vec4i_32>;

  ArrayVectorType pythagoreanTriples;
  pythagoreanTriples.Allocate(5);
  {
    auto inputPortal = pythagoreanTriples.WritePortal();

    inputPortal.Set(0, vtkm::make_Vec(3, 4, 5, 0));
    inputPortal.Set(1, vtkm::make_Vec(5, 12, 13, 0));
    inputPortal.Set(2, vtkm::make_Vec(8, 15, 17, 0));
    inputPortal.Set(3, vtkm::make_Vec(7, 24, 25, 0));
    inputPortal.Set(4, vtkm::make_Vec(9, 40, 41, 0));
  }

  vtkm::worklet::DispatcherMapField<vtkm::worklet::Magnitude> dispatcher(magnitudeWorklet);

  ArrayReturnType result;

  dispatcher.Invoke(pythagoreanTriples, result);

  auto inputPortal = pythagoreanTriples.ReadPortal();
  auto resultPortal = result.ReadPortal();
  for (vtkm::Id i = 0; i < result.GetNumberOfValues(); ++i)
  {
    vtkm::Vec4i_32 inValue = inputPortal.Get(i);
    vtkm::Float64 expectedValue =
      std::sqrt((inValue[0] * inValue[0]) + (inValue[1] * inValue[1]) + (inValue[2] * inValue[2]));
    vtkm::Float64 resultValue = resultPortal.Get(i);
    VTKM_TEST_ASSERT(test_equal(expectedValue, resultValue), expectedValue, " != ", resultValue);
  }
}
}

int UnitTestMagnitude(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMagnitude, argc, argv);
}
