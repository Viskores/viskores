//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/exec/arg/BasicArg.h>
#include <viskores/exec/arg/WorkIndex.h>

#include <viskores/testing/Testing.h>

namespace
{

void TestExecutionSignatures()
{
  VISKORES_IS_EXECUTION_SIGNATURE_TAG(viskores::exec::arg::BasicArg<1>);

  VISKORES_TEST_ASSERT(
    viskores::exec::arg::internal::ExecutionSignatureTagCheck<viskores::exec::arg::BasicArg<2>>::Valid,
    "Bad check for BasicArg");

  VISKORES_TEST_ASSERT(
    viskores::exec::arg::internal::ExecutionSignatureTagCheck<viskores::exec::arg::WorkIndex>::Valid,
    "Bad check for WorkIndex");

  VISKORES_TEST_ASSERT(!viskores::exec::arg::internal::ExecutionSignatureTagCheck<viskores::Id>::Valid,
                   "Bad check for viskores::Id");
}

} // anonymous namespace

int UnitTestExecutionSignatureTag(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestExecutionSignatures, argc, argv);
}
