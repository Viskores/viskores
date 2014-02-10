//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>

#include <vtkm/cont/ArrayContainerControlBasic.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TemplatedTests
{
  typedef vtkm::cont::internal::ArrayManagerExecutionShareWithControl
      <T, vtkm::cont::ArrayContainerControlTagBasic>
      ArrayManagerType;
  typedef typename ArrayManagerType::ValueType ValueType;
  typedef vtkm::cont::internal::ArrayContainerControl<
      T, vtkm::cont::ArrayContainerControlTagBasic> ArrayContainerType;

  void SetContainer(ArrayContainerType &array, ValueType value)
  {
    std::fill(array.GetPortal().GetIteratorBegin(),
              array.GetPortal().GetIteratorEnd(),
              value);
  }

  template <class IteratorType>
  bool CheckArray(IteratorType begin, IteratorType end, ValueType value)
  {
    for (IteratorType iter = begin; iter != end; iter++)
      {
      if (!test_equal(*iter, value)) return false;
      }
    return true;
  }

  bool CheckContainer(ArrayContainerType &array, ValueType value)
  {
    return CheckArray(array.GetPortalConst().GetIteratorBegin(),
                      array.GetPortalConst().GetIteratorEnd(),
                      value);
  }

  void InputData()
  {
    const ValueType INPUT_VALUE(4145);

    ArrayContainerType controlArray;
    controlArray.Allocate(ARRAY_SIZE);
    SetContainer(controlArray, INPUT_VALUE);

    ArrayManagerType executionArray;
    executionArray.LoadDataForInput(controlArray.GetPortalConst());

    // Although the ArrayManagerExecutionShareWithControl class wraps the
    // control array portal in a different array portal, it should still
    // give the same iterator (to avoid any unnecessary indirection).
    VTKM_TEST_ASSERT(
          controlArray.GetPortalConst().GetIteratorBegin() ==
          executionArray.GetPortalConst().GetIteratorBegin(),
          "Execution array manager not holding control array iterators.");

    std::vector<ValueType> copyBack(ARRAY_SIZE);
    executionArray.CopyInto(copyBack.begin());

    VTKM_TEST_ASSERT(CheckArray(copyBack.begin(), copyBack.end(), INPUT_VALUE),
                    "Did not get correct array back.");
  }

  void InPlaceData()
  {
    const ValueType INPUT_VALUE(2350);

    ArrayContainerType controlArray;
    controlArray.Allocate(ARRAY_SIZE);
    SetContainer(controlArray, INPUT_VALUE);

    ArrayManagerType executionArray;
    executionArray.LoadDataForInPlace(controlArray.GetPortal());

    // Although the ArrayManagerExecutionShareWithControl class wraps the
    // control array portal in a different array portal, it should still
    // give the same iterator (to avoid any unnecessary indirection).
    VTKM_TEST_ASSERT(
          controlArray.GetPortal().GetIteratorBegin() ==
          executionArray.GetPortal().GetIteratorBegin(),
          "Execution array manager not holding control array iterators.");
    VTKM_TEST_ASSERT(
          controlArray.GetPortalConst().GetIteratorBegin() ==
          executionArray.GetPortalConst().GetIteratorBegin(),
          "Execution array manager not holding control array iterators.");

    std::vector<ValueType> copyBack(ARRAY_SIZE);
    executionArray.CopyInto(copyBack.begin());

    VTKM_TEST_ASSERT(CheckArray(copyBack.begin(), copyBack.end(), INPUT_VALUE),
                    "Did not get correct array back.");
  }

  void OutputData()
  {
    const ValueType OUTPUT_VALUE(6712);

    ArrayContainerType controlArray;

    ArrayManagerType executionArray;
    executionArray.AllocateArrayForOutput(controlArray, ARRAY_SIZE);

    std::fill(executionArray.GetPortal().GetIteratorBegin(),
              executionArray.GetPortal().GetIteratorEnd(),
              OUTPUT_VALUE);

    std::vector<ValueType> copyBack(ARRAY_SIZE);
    executionArray.CopyInto(copyBack.begin());

    VTKM_TEST_ASSERT(CheckArray(copyBack.begin(), copyBack.end(), OUTPUT_VALUE),
                    "Did not get correct array back.");

    executionArray.RetrieveOutputData(controlArray);

    VTKM_TEST_ASSERT(CheckContainer(controlArray, OUTPUT_VALUE),
                    "Did not get the right value in the control container.");
  }

  void operator()() {

    InputData();
    InPlaceData();
    OutputData();

  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T)
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayManagerShare()
{
  vtkm::testing::Testing::TryAllTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestArrayManagerExecutionShareWithControl(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayManagerShare);
}
