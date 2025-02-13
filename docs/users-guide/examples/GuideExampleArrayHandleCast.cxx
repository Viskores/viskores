//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayHandleCast.h>

#include <vector>

#include <viskores/cont/testing/Testing.h>

namespace
{

template<typename OriginalType, typename ArrayHandleType>
void CheckArray(const ArrayHandleType array)
{
  viskores::Id length = array.GetNumberOfValues();

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == length, "Portal has wrong size.");

  for (viskores::Id index = 0; index < length; index++)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(index), TestValue(index, OriginalType())),
                         "Array has wrong value.");
    VISKORES_TEST_ASSERT(
      !test_equal(portal.Get(index),
                  TestValue(index, typename ArrayHandleType::ValueType())),
      "Array has wrong value.");
  }
}

////
//// BEGIN-EXAMPLE ArrayHandleCast
////
template<typename T>
VISKORES_CONT void Foo(const std::vector<T>& inputData)
{
  viskores::cont::ArrayHandle<T> originalArray =
    viskores::cont::make_ArrayHandle(inputData, viskores::CopyFlag::On);

  viskores::cont::ArrayHandleCast<viskores::Float64, viskores::cont::ArrayHandle<T>>
    castArray(originalArray);
  ////
  //// END-EXAMPLE ArrayHandleCast
  ////
  CheckArray<T>(castArray);

  CheckArray<T>(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleCast
    ////
    viskores::cont::make_ArrayHandleCast<viskores::Float64>(originalArray)
    ////
    //// END-EXAMPLE MakeArrayHandleCast
    ////
  );
}

void Test()
{
  const std::size_t ARRAY_SIZE = 50;
  std::vector<viskores::Int32> inputData(ARRAY_SIZE);
  for (std::size_t index = 0; index < ARRAY_SIZE; index++)
  {
    inputData[index] = TestValue(viskores::Id(index), viskores::Int32());
  }

  Foo(inputData);
}

} // anonymous namespace

int GuideExampleArrayHandleCast(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
