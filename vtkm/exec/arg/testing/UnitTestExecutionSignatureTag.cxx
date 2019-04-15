//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

int UnitTestExecutionSignatureTag(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestExecutionSignatures, argc, argv);
}
