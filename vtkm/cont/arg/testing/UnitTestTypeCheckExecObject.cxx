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

int UnitTestTypeCheckExecObject(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCheckExecObject);
}
