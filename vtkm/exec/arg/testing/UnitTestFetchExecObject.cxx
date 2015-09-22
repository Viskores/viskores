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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/arg/FetchTagExecObject.h>

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/exec/ExecutionObjectBase.h>

#include <vtkm/testing/Testing.h>

#define EXPECTED_NUMBER 67

namespace {

struct TestExecutionObject : public vtkm::exec::ExecutionObjectBase
{
  TestExecutionObject() : Number( static_cast<vtkm::Int32>(0xDEADDEAD) ) {  }
  TestExecutionObject(vtkm::Int32 number) : Number(number) {  }
  vtkm::Int32 Number;
};

template<vtkm::IdComponent ParamIndex, typename Invocation>
void TryInvocation(const Invocation &invocation)
{
  typedef vtkm::exec::arg::Fetch<
      vtkm::exec::arg::FetchTagExecObject,
      vtkm::exec::arg::AspectTagDefault,
      vtkm::exec::arg::ThreadIndicesBasic,
      TestExecutionObject> FetchType;

  FetchType fetch;

  vtkm::exec::arg::ThreadIndicesBasic indices(0, invocation);

  TestExecutionObject execObject = fetch.Load(
        indices, invocation.Parameters.template GetParameter<ParamIndex>());
  VTKM_TEST_ASSERT(execObject.Number == EXPECTED_NUMBER,
                   "Did not load object correctly.");

  execObject.Number = -1;

  // This should be a no-op.
  fetch.Store(
        indices,
        invocation.Parameters.template GetParameter<ParamIndex>(),
        execObject);

  // Data in Invocation should not have changed.
  execObject = invocation.Parameters.template GetParameter<ParamIndex>();
  VTKM_TEST_ASSERT(execObject.Number == EXPECTED_NUMBER,
                   "Fetch changed read-only execution object.");
}

template<vtkm::IdComponent ParamIndex>
void TryParamIndex()
{
  std::cout << "Trying ExecObject fetch on parameter " << ParamIndex
            << std::endl;

  typedef vtkm::internal::FunctionInterface<
      void(vtkm::internal::NullType,
           vtkm::internal::NullType,
           vtkm::internal::NullType,
           vtkm::internal::NullType,
           vtkm::internal::NullType)>
      BaseFunctionInterface;

  TryInvocation<ParamIndex>(vtkm::internal::make_Invocation<1>(
                              BaseFunctionInterface().Replace<ParamIndex>(
                                TestExecutionObject(EXPECTED_NUMBER)),
                              vtkm::internal::NullType(),
                              vtkm::internal::NullType()));
}

void TestExecObjectFetch()
{
  TryParamIndex<1>();
  TryParamIndex<2>();
  TryParamIndex<3>();
  TryParamIndex<4>();
  TryParamIndex<5>();
}

} // anonymous namespace

int UnitTestFetchExecObject(int, char *[])
{
  return vtkm::testing::Testing::Run(TestExecObjectFetch);
}
