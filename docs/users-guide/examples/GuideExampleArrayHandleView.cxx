//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleView.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(const ArrayHandleType array,
                typename ArrayHandleType::ValueType firstValue,
                vtkm::Id expectedLength)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == expectedLength, "Array has wrong size.");

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == expectedLength,
                   "Portal has wrong size.");

  typename ArrayHandleType::ValueType expectedValue = firstValue;
  for (vtkm::Id index = 0; index < expectedLength; index++)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(index), expectedValue),
                     "Array has wrong value.");
    expectedValue++;
  }
}

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleView
  ////
  vtkm::cont::ArrayHandle<vtkm::Id> sourceArray;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(10), sourceArray);
  // sourceArray has [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

  vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::Id>> viewArray(
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
    vtkm::cont::make_ArrayHandleView(sourceArray, 3, 5)
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
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
