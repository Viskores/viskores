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

#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <cstring>
#include <vtkm/testing/Testing.h>

namespace
{

void TestErrorMessageBuffer()
{
  char messageBuffer[100];

  std::cout << "Testing buffer large enough for message." << std::endl;
  messageBuffer[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer largeBuffer(messageBuffer, 100);
  VTKM_TEST_ASSERT(!largeBuffer.IsErrorRaised(), "Message created with error.");

  largeBuffer.RaiseError("Hello World");
  VTKM_TEST_ASSERT(largeBuffer.IsErrorRaised(), "Error not reported.");
  VTKM_TEST_ASSERT(strcmp(messageBuffer, "Hello World") == 0, "Did not record error message.");

  std::cout << "Testing truncated error message." << std::endl;
  messageBuffer[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer smallBuffer(messageBuffer, 9);
  VTKM_TEST_ASSERT(!smallBuffer.IsErrorRaised(), "Message created with error.");

  smallBuffer.RaiseError("Hello World");
  VTKM_TEST_ASSERT(smallBuffer.IsErrorRaised(), "Error not reported.");
  VTKM_TEST_ASSERT(strcmp(messageBuffer, "Hello Wo") == 0, "Did not record error message.");
}

} // anonymous namespace

int UnitTestErrorMessageBuffer(int, char* [])
{
  return (vtkm::testing::Testing::Run(TestErrorMessageBuffer));
}
