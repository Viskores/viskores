//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/FetchTagExecObject.h>

#include <vtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <vtkm/testing/Testing.h>

#define EXPECTED_NUMBER 67

namespace
{

struct TestExecutionObject
{
  TestExecutionObject()
    : Number(static_cast<vtkm::Int32>(0xDEADDEAD))
  {
  }
  TestExecutionObject(vtkm::Int32 number)
    : Number(number)
  {
  }
  vtkm::Int32 Number;
};

void TryInvocation()
{
  TestExecutionObject execObjectStore(EXPECTED_NUMBER);

  using FetchType = vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagExecObject,
                                           vtkm::exec::arg::AspectTagDefault,
                                           vtkm::exec::arg::ThreadIndicesTesting,
                                           TestExecutionObject>;

  FetchType fetch;

  vtkm::exec::arg::ThreadIndicesTesting indices(0);

  TestExecutionObject execObject = fetch.Load(indices, execObjectStore);
  VTKM_TEST_ASSERT(execObject.Number == EXPECTED_NUMBER, "Did not load object correctly.");

  execObject.Number = -1;

  // This should be a no-op.
  fetch.Store(indices, execObjectStore, execObject);

  // Data in Invocation should not have changed.
  VTKM_TEST_ASSERT(execObjectStore.Number == EXPECTED_NUMBER,
                   "Fetch changed read-only execution object.");
}

void TestExecObjectFetch()
{
  TryInvocation();
}

} // anonymous namespace

int UnitTestFetchExecObject(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch, argc, argv);
}
