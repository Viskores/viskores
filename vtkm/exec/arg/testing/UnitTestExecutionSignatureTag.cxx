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

#include <vtkm/exec/arg/BasicArg.h>
#include <vtkm/exec/arg/WorkIndex.h>

#include <vtkm/testing/Testing.h>

namespace
{

void TestExecutionSignatures()
{
  VTKM_IS_EXECUTION_SIGNATURE_TAG(vtkm::exec::arg::BasicArg<1>);

  VTKM_TEST_ASSERT(
    vtkm::exec::arg::internal::ExecutionSignatureTagCheck<vtkm::exec::arg::BasicArg<2>>::Valid,
    "Bad check for BasicArg");

  VTKM_TEST_ASSERT(
    vtkm::exec::arg::internal::ExecutionSignatureTagCheck<vtkm::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  VTKM_TEST_ASSERT(!vtkm::exec::arg::internal::ExecutionSignatureTagCheck<vtkm::Id>::Valid,
                   "Bad check for vtkm::Id");
}

} // anonymous namespace

int UnitTestExecutionSignatureTag(int, char* [])
{
  return vtkm::testing::Testing::Run(TestExecutionSignatures);
}
