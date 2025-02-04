//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/exec/internal/ErrorMessageBuffer.h>

#include <cstring>
#include <viskores/testing/Testing.h>

namespace
{

void TestErrorMessageBuffer()
{
  char messageBuffer[100];

  std::cout << "Testing buffer large enough for message." << std::endl;
  messageBuffer[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer largeBuffer(messageBuffer, 100);
  VISKORES_TEST_ASSERT(!largeBuffer.IsErrorRaised(), "Message created with error.");

  largeBuffer.RaiseError("Hello World");
  VISKORES_TEST_ASSERT(largeBuffer.IsErrorRaised(), "Error not reported.");
  VISKORES_TEST_ASSERT(strcmp(messageBuffer, "Hello World") == 0, "Did not record error message.");

  std::cout << "Testing truncated error message." << std::endl;
  messageBuffer[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer smallBuffer(messageBuffer, 9);
  VISKORES_TEST_ASSERT(!smallBuffer.IsErrorRaised(), "Message created with error.");

  smallBuffer.RaiseError("Hello World");
  VISKORES_TEST_ASSERT(smallBuffer.IsErrorRaised(), "Error not reported.");
  VISKORES_TEST_ASSERT(strcmp(messageBuffer, "Hello Wo") == 0, "Did not record error message.");
}

} // anonymous namespace

int UnitTestErrorMessageBuffer(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(TestErrorMessageBuffer, argc, argv);
}
