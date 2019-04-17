//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>

#include <vtkm/exec/arg/testing/ThreadIndicesTesting.h>

#include <vtkm/testing/Testing.h>

namespace
{

void TestWorkIndexFetch()
{
  std::cout << "Trying WorkIndex fetch." << std::endl;

  using FetchType =
    vtkm::exec::arg::Fetch<vtkm::exec::arg::FetchTagArrayDirectIn, // Not used but probably common.
                           vtkm::exec::arg::AspectTagWorkIndex,
                           vtkm::exec::arg::ThreadIndicesTesting,
                           vtkm::internal::NullType>;

  FetchType fetch;

  for (vtkm::Id index = 0; index < 10; index++)
  {
    vtkm::exec::arg::ThreadIndicesTesting indices(index);

    vtkm::Id value = fetch.Load(indices, vtkm::internal::NullType());
    VTKM_TEST_ASSERT(value == index, "Fetch did not give correct work index.");

    value++;

    // This should be a no-op.
    fetch.Store(indices, vtkm::internal::NullType(), value);
  }
}

} // anonymous namespace

int UnitTestFetchWorkIndex(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestWorkIndexFetch, argc, argv);
}
