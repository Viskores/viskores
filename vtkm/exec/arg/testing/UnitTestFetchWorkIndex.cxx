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

#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/internal/Invocation.h>

#include <vtkm/testing/Testing.h>

namespace {

struct NullParam {  };

template<typename Invocation>
void TryInvocation(const Invocation &invocation)
{
  typedef vtkm::exec::arg::Fetch<
      vtkm::exec::arg::FetchTagArrayDirectIn, // Not used but probably common.
      vtkm::exec::arg::AspectTagWorkIndex,
      vtkm::exec::arg::ThreadIndicesBasic,
      NullParam> FetchType;

  FetchType fetch;

  for (vtkm::Id index = 0; index < 10; index++)
  {
    vtkm::exec::arg::ThreadIndicesBasic indices(index, invocation);

    vtkm::Id value = fetch.Load(indices, NullParam());
    VTKM_TEST_ASSERT(value == index,
                     "Fetch did not give correct work index.");

    value++;

    // This should be a no-op.
    fetch.Store(indices, NullParam(), value);
  }
}

void TestWorkIndexFetch()
{
  std::cout << "Trying WorkIndex fetch." << std::endl;

  typedef vtkm::internal::FunctionInterface<
      void(NullParam,NullParam,NullParam,NullParam,NullParam)>
      BaseFunctionInterface;

  TryInvocation(vtkm::internal::make_Invocation<1>(BaseFunctionInterface(),
                                                   NullParam(),
                                                   NullParam()));
}

} // anonymous namespace

int UnitTestFetchWorkIndex(int, char *[])
{
  return vtkm::testing::Testing::Run(TestWorkIndexFetch);
}
