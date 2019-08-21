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
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void RecursiveFunction(int recurse)
{
  if (recurse < 5)
  {
    RecursiveFunction(++recurse);
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("Too much recursion");
  }
}

void ValidateError(const vtkm::cont::Error& error)
{
  std::string message = "Too much recursion";
  std::stringstream stackTraceStream(error.GetStackTrace());
  std::string tmp;
  size_t count = 0;
  while (std::getline(stackTraceStream, tmp))
  {
    count++;
  }

  VTKM_TEST_ASSERT(test_equal(message, error.GetMessage()), "Message was incorrect");
  VTKM_TEST_ASSERT(count > 11, "StackTrace did not recurse enough");
  VTKM_TEST_ASSERT(test_equal(message + "\n" + error.GetStackTrace(), std::string(error.what())),
                   "what() was incorrect");
}

void DoErrorTest()
{
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Check base error mesgs");
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
