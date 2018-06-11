//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/arg/FetchTagExecObject.h>

#include <vtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/testing/Testing.h>

#define EXPECTED_NUMBER 67

namespace
{

struct TestExecutionObject : public vtkm::cont::ExecutionObjectBase
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

int UnitTestFetchExecObject(int, char* [])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch);
}
