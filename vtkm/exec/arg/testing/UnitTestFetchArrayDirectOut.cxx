//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

#include <vtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <vtkm/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

static vtkm::Id g_NumSets;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    VTKM_TEST_ASSERT(test_equal(value, TestValue(index, ValueType())),
                     "Tried to set invalid value.");
    g_NumSets++;
  }
};

template <typename T>
struct FetchArrayDirectOutTests
{

  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayDirectOut,
                                             vtkm::exec::arg::AspectTagDefault,
                                             vtkm::exec::arg::ThreadIndicesTesting,
                                             TestPortal<T>>;

    FetchType fetch;

    g_NumSets = 0;

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      vtkm::exec::arg::ThreadIndicesTesting indices(index);

      // This is a no-op, but should be callable.
      T value = fetch.Load(indices, execObject);

      value = TestValue(index, T());

      // The portal will check to make sure we are setting a good value.
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
    FetchArrayDirectOutTests<T>()();
  }
};

void TestExecObjectFetch()
{
  vtkm::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectOut(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
