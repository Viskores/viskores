//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleView.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(const ArrayHandleType array,
                typename ArrayHandleType::ValueType firstValue,
                viskores::Id expectedLength)
{
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == expectedLength, "Array has wrong size.");

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == expectedLength,
                   "Portal has wrong size.");

  typename ArrayHandleType::ValueType expectedValue = firstValue;
  for (viskores::Id index = 0; index < expectedLength; index++)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(index), expectedValue),
                     "Array has wrong value.");
    expectedValue++;
  }
}

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleView
  ////
  viskores::cont::ArrayHandle<viskores::Id> sourceArray;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(10), sourceArray);
  // sourceArray has [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

  viskores::cont::ArrayHandleView<viskores::cont::ArrayHandle<viskores::Id>> viewArray(
    sourceArray, 3, 5);
  // viewArray has [3, 4, 5, 6, 7]
  ////
  //// END-EXAMPLE ArrayHandleView
  ////

  CheckArray(viewArray, 3, 5);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleView
    ////
    viskores::cont::make_ArrayHandleView(sourceArray, 3, 5)
    ////
    //// END-EXAMPLE MakeArrayHandleView
    ////
    ,
    3,
    5);
}

} // anonymous namespace

int GuideExampleArrayHandleView(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
