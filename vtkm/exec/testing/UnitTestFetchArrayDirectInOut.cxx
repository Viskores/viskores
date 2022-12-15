//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>

#include <vtkm/exec/testing/ThreadIndicesTesting.h>

#include <vtkm/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

static vtkm::Id g_NumSets;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  ValueType Get(vtkm::Id index) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }

  void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    VTKM_TEST_ASSERT(test_equal(value, ValueType(2) * TestValue(index, ValueType())),
                     "Tried to set invalid value.");
    g_NumSets++;
  }
};

template <typename T>
struct FetchArrayDirectInTests
{

  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayDirectInOut,
                                             vtkm::exec::arg::AspectTagDefault,
                                             TestPortal<T>>;

    FetchType fetch;

    g_NumSets = 0;

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      vtkm::exec::arg::ThreadIndicesTesting indices(index);

      T value = fetch.Load(indices, execObject);
      VTKM_TEST_ASSERT(test_equal(value, TestValue(index, T())), "Got invalid value from Load.");

      value = T(T(2) * value);

      fetch.Store(indices, execObject, value);
    }

    VTKM_TEST_ASSERT(g_NumSets == ARRAY_SIZE,
                     "Array portal's set not called correct number of times."
                     "Store method must be wrong.");
  }
};

struct TryType
{
  template <typename T>
  void operator()(T) const
  {
    FetchArrayDirectInTests<T>()();
  }
};

void TestExecObjectFetch()
{
  vtkm::testing::Testing::TryTypes(TryType(), vtkm::TypeListCommon());
}

} // anonymous namespace

int UnitTestFetchArrayDirectInOut(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
