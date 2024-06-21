//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCast.h>

#include <vector>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template<typename OriginalType, typename ArrayHandleType>
void CheckArray(const ArrayHandleType array)
{
  vtkm::Id length = array.GetNumberOfValues();

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == length, "Portal has wrong size.");

  for (vtkm::Id index = 0; index < length; index++)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(index), TestValue(index, OriginalType())),
                     "Array has wrong value.");
    VTKM_TEST_ASSERT(
      !test_equal(portal.Get(index),
                  TestValue(index, typename ArrayHandleType::ValueType())),
      "Array has wrong value.");
  }
}

////
//// BEGIN-EXAMPLE ArrayHandleCast
////
template<typename T>
VTKM_CONT void Foo(const std::vector<T>& inputData)
{
  vtkm::cont::ArrayHandle<T> originalArray =
    vtkm::cont::make_ArrayHandle(inputData, vtkm::CopyFlag::On);

  vtkm::cont::ArrayHandleCast<vtkm::Float64, vtkm::cont::ArrayHandle<T>> castArray(
    originalArray);
  ////
  //// END-EXAMPLE ArrayHandleCast
  ////
  CheckArray<T>(castArray);

  CheckArray<T>(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleCast
    ////
    vtkm::cont::make_ArrayHandleCast<vtkm::Float64>(originalArray)
    ////
    //// END-EXAMPLE MakeArrayHandleCast
    ////
  );
}

void Test()
{
  const std::size_t ARRAY_SIZE = 50;
  std::vector<vtkm::Int32> inputData(ARRAY_SIZE);
  for (std::size_t index = 0; index < ARRAY_SIZE; index++)
  {
    inputData[index] = TestValue(vtkm::Id(index), vtkm::Int32());
  }

  Foo(inputData);
}

} // anonymous namespace

int GuideExampleArrayHandleCast(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
