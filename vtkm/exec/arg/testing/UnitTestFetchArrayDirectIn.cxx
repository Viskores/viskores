//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>

#include <vtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <vtkm/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TestPortal
{
  using ValueType = T;

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    VTKM_TEST_ASSERT(index >= 0, "Bad portal index.");
    VTKM_TEST_ASSERT(index < this->GetNumberOfValues(), "Bad portal index.");
    return TestValue(index, ValueType());
  }
};

template <typename T>
struct FetchArrayDirectInTests
{
  void operator()()
  {
    TestPortal<T> execObject;

    using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayDirectIn,
                                             vtkm::exec::arg::AspectTagDefault,
                                             vtkm::exec::arg::ThreadIndicesTesting,
                                             TestPortal<T>>;

    FetchType fetch;

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      vtkm::exec::arg::ThreadIndicesTesting indices(index);

      T value = fetch.Load(indices, execObject);
      VTKM_TEST_ASSERT(test_equal(value, TestValue(index, T())), "Got invalid value from Load.");

      value = T(T(2) * value);

      // This should be a no-op, but we should be able to call it.
      fetch.Store(indices, execObject, value);
    }
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
  vtkm::testing::Testing::TryTypes(TryType());
}

} // anonymous namespace

int UnitTestFetchArrayDirectIn(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
