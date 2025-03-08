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

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(const ArrayHandleType array,
                typename ArrayHandleType::ValueType startValue,
                typename ArrayHandleType::ValueType stepValue,
                viskores::Id expectedLength)
{
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == expectedLength,
                       "Array has wrong size.");

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == expectedLength,
                       "Portal has wrong size.");

  typename ArrayHandleType::ValueType expectedValue = startValue;
  for (viskores::Id index = 0; index < expectedLength; index++)
  {
    VISKORES_TEST_ASSERT(test_equal(portal.Get(index), expectedValue),
                         "Array has wrong value.");
    expectedValue = expectedValue + stepValue;
  }
}

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleIndex
  ////
  // Create an array containing [0, 1, 2, 3, ..., 49].
  viskores::cont::ArrayHandleIndex indexArray(50);
  ////
  //// END-EXAMPLE ArrayHandleIndex
  ////
  CheckArray(indexArray, 0, 1, 50);

  ////
  //// BEGIN-EXAMPLE ArrayHandleCountingBasic
  ////
  // Create an array containing [-1.0, -0.9, -0.8, ..., 0.9, 1.0]
  viskores::cont::ArrayHandleCounting<viskores::Float32> sampleArray(-1.0f, 0.1f, 21);
  ////
  //// END-EXAMPLE ArrayHandleCountingBasic
  ////
  CheckArray(sampleArray, -1.0f, 0.1f, 21);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleCountingBasic
    ////
    // Create an array containing [-1.0, -0.9, -0.8, ..., 0.9, 1.0]
    viskores::cont::make_ArrayHandleCounting(-1.0f, 0.1f, 21)
    ////
    //// END-EXAMPLE MakeArrayHandleCountingBasic
    ////
    ,
    -1.0f,
    0.1f,
    21);
  ////
  //// BEGIN-EXAMPLE ArrayHandleCountingBackward
  ////
  // Create an array containing [49, 48, 47, 46, ..., 0].
  viskores::cont::ArrayHandleCounting<viskores::Id> backwardIndexArray(49, -1, 50);
  ////
  //// END-EXAMPLE ArrayHandleCountingBackward
  ////
  CheckArray(backwardIndexArray, 49, -1, 50);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE ArrayHandleCountingVec
    ////
    // Create an array containg [(0,-3,75), (1,2,25), (3,7,-25)]
    viskores::cont::make_ArrayHandleCounting(
      viskores::make_Vec(0, -3, 75), viskores::make_Vec(1, 5, -50), 3)
    ////
    //// END-EXAMPLE ArrayHandleCountingVec
    ////
    ,
    viskores::make_Vec(0, -3, 75),
    viskores::make_Vec(1, 5, -50),
    3);
}

} // anonymous namespace

int GuideExampleArrayHandleCounting(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
