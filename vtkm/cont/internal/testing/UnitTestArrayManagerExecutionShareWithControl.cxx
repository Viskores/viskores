//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/ArrayManagerExecutionShareWithControl.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TemplatedTests
{
  using ArrayManagerType =
    vtkm::cont::internal::ArrayManagerExecutionShareWithControl<T, vtkm::cont::StorageTagBasic>;
  using ValueType = typename ArrayManagerType::ValueType;
  using StorageType = vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagBasic>;

  void SetStorage(StorageType& array, const ValueType& value)
  {
    vtkm::cont::ArrayPortalToIterators<typename StorageType::PortalType> iterators(
      array.GetPortal());
    std::fill(iterators.GetBegin(), iterators.GetEnd(), value);
  }

  template <class PortalType>
  bool CheckPortal(const PortalType& portal, const ValueType& value)
  {
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
    {
      if (!test_equal(portal.Get(index), value))
      {
        return false;
      }
    }
    return true;
  }

  bool CheckStorage(StorageType& array, const ValueType& value)
  {
    return CheckPortal(array.GetPortalConst(), value);
  }

  bool CheckManager(ArrayManagerType& manager, const ValueType& value)
  {
    return CheckPortal(manager.PrepareForInput(false), value);
  }

  void InputData()
  {
    const ValueType INPUT_VALUE(45);

    StorageType storage;
    storage.Allocate(ARRAY_SIZE);
    SetStorage(storage, INPUT_VALUE);

    ArrayManagerType executionArray(&storage);

    // Although the ArrayManagerExecutionShareWithControl class wraps the
    // control array portal in a different array portal, it should still
    // give the same iterator (to avoid any unnecessary indirection).
    VTKM_TEST_ASSERT(vtkm::cont::ArrayPortalToIteratorBegin(storage.GetPortalConst()) ==
                       vtkm::cont::ArrayPortalToIteratorBegin(executionArray.PrepareForInput(true)),
                     "Execution array manager not holding control array iterators.");

    VTKM_TEST_ASSERT(CheckManager(executionArray, INPUT_VALUE), "Did not get correct array back.");
  }

  void InPlaceData()
  {
    const ValueType INPUT_VALUE(50);

    StorageType storage;
    storage.Allocate(ARRAY_SIZE);
    SetStorage(storage, INPUT_VALUE);

    ArrayManagerType executionArray(&storage);

    // Although the ArrayManagerExecutionShareWithControl class wraps the
    // control array portal in a different array portal, it should still
    // give the same iterator (to avoid any unnecessary indirection).
    VTKM_TEST_ASSERT(
      vtkm::cont::ArrayPortalToIteratorBegin(storage.GetPortal()) ==
        vtkm::cont::ArrayPortalToIteratorBegin(executionArray.PrepareForInPlace(true)),
      "Execution array manager not holding control array iterators.");
    VTKM_TEST_ASSERT(
      vtkm::cont::ArrayPortalToIteratorBegin(storage.GetPortalConst()) ==
        vtkm::cont::ArrayPortalToIteratorBegin(executionArray.PrepareForInput(false)),
      "Execution array manager not holding control array iterators.");

    VTKM_TEST_ASSERT(CheckManager(executionArray, INPUT_VALUE), "Did not get correct array back.");
  }

  void OutputData()
  {
    const ValueType OUTPUT_VALUE(12);

    StorageType storage;

    ArrayManagerType executionArray(&storage);

    vtkm::cont::ArrayPortalToIterators<typename ArrayManagerType::PortalType> iterators(
      executionArray.PrepareForOutput(ARRAY_SIZE));
    std::fill(iterators.GetBegin(), iterators.GetEnd(), OUTPUT_VALUE);

    VTKM_TEST_ASSERT(CheckManager(executionArray, OUTPUT_VALUE), "Did not get correct array back.");

    executionArray.RetrieveOutputData(&storage);

    VTKM_TEST_ASSERT(CheckStorage(storage, OUTPUT_VALUE),
                     "Did not get the right value in the storage.");
  }

  void operator()()
  {

    InputData();
    InPlaceData();
    OutputData();
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayManagerShare()
{
  vtkm::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestArrayManagerExecutionShareWithControl(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayManagerShare, argc, argv);
}
