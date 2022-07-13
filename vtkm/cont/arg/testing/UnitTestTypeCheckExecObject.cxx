//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This test does not really need a device compiler
#define VTKM_NO_ERROR_ON_MIXED_CUDA_CXX_TAG

#include <vtkm/cont/arg/TypeCheckTagExecObject.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

struct TestExecutionObject : vtkm::cont::ExecutionObjectBase
{
};
struct TestNotExecutionObject
{
};

void TestCheckExecObject()
{
  std::cout << "Checking reporting of type checking exec object." << std::endl;

  using vtkm::cont::arg::TypeCheck;
  using vtkm::cont::arg::TypeCheckTagExecObject;

  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagExecObject, TestExecutionObject>::value),
                   "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, TestNotExecutionObject>::value),
                   "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, vtkm::Id>::value), "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagExecObject, vtkm::cont::ArrayHandle<vtkm::Id>>::value),
                   "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckExecObject(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCheckExecObject, argc, argv);
}
