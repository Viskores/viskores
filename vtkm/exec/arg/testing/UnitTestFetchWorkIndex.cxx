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

int UnitTestFetchWorkIndex(int, char* [])
{
  return vtkm::testing::Testing::Run(TestWorkIndexFetch);
}
