//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Error.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void RecursiveFunction(int recurse)
{
  if (recurse < 5)
  {
    RecursiveFunction(recurse++);
  }
  else
  {
    throw vtkm::cont::Error("Too much recursion");
  }
}

void ValidateError(const vtkm::cont::Error& error)
{
  std::cout << error.what() << std::endl;
  std::string stackTrace = "";
  std::string message = "Too much recursion";
  VTKM_TEST_ASSERT(test_equal(message, error.GetMessage()), "Message was incorrect");
  VTKM_TEST_ASSERT(test_equal(stackTrace, error.GetStackTrace()), "StackTrace was incorrect");
  VTKM_TEST_ASSERT(test_equal((message + "\n" + stackTrace).c_str(), error.what()),
                   "what() was incorrect");
}

void DoErrorTest()
{
  std::cout << "Check base error msgs" << std::endl;
  try
  {
    RecursiveFunction(0);
  }
  catch (const vtkm::cont::Error& e)
  {
    ValidateError(e);
  }
}

} // anonymous namespace

int UnitTestError(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoErrorTest, argc, argv);
}
